// Copyright (C) 2018-2023 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef _NBL_EXT_IES_INCLUDED_
#define _NBL_EXT_IES_INCLUDED_

#include "nabla.h"
#include "nbl/video/IGPUShader.h"
#include "nbl/asset/ICPUShader.h"
#include "nbl/asset/ICPUImage.h"

#include "nbl/asset/IAssetManager.h"

#include "nbl/asset/interchange/IAssetLoader.h"

namespace nbl
{
namespace ext
{
namespace IES
{

class CIESProfile {
public:
    enum PhotometricType : uint32_t
    {
        TYPE_NONE,
        TYPE_C,
        TYPE_B,
        TYPE_A,
    };

    CIESProfile() = default;
    CIESProfile(PhotometricType type, size_t hSize, size_t vSize)
        : type(type), hAngles(hSize), vAngles(vSize), data(hSize*vSize) {}
    ~CIESProfile() = default;
    core::vector<double>& getHoriAngles() { return hAngles; }
    const core::vector<double>& getHoriAngles() const { return hAngles; }
    core::vector<double>& getVertAngles() { return vAngles; }
    const core::vector<double>& getVertAngles() const { return vAngles; }
    size_t getHoriSize() const { return hAngles.size(); }
    size_t getVertSize() const { return vAngles.size(); }
    double setValue(size_t i, size_t j, double val) { data[getVertSize() * i + j] = val;  }
    double getValue(size_t i, size_t j) const { return data[getVertSize() * i + j]; }
    double sample(double vAngle, double hAngle) const {
        // warp angle
        vAngle = fmod(vAngle, vAngles.back());
        hAngle = hAngles.back() == 0.0 ? 0.0 : fmod(hAngle, hAngles.back()); // when last horizontal angle is zero it's symmetric across all planes
        // bilinear interpolation
        int i0 = std::lower_bound(std::begin(vAngles), std::end(vAngles), vAngle) - std::begin(vAngles);
        int i1 = std::upper_bound(std::begin(vAngles), std::end(vAngles), vAngle) - std::begin(vAngles);
        int j0 = std::lower_bound(std::begin(hAngles), std::end(hAngles), hAngle) - std::begin(hAngles);
        int j1 = std::upper_bound(std::begin(hAngles), std::end(hAngles), hAngle) - std::begin(hAngles);
        double u = (hAngle - hAngles[j0]) / (hAngles[j1] - hAngles[j0]);
        double v = (vAngle - vAngles[i0]) / (vAngles[i1] - vAngles[i0]);
        double s0 = getValue(i0, j0) * (1.0 - u) + getValue(i0, j1) * u;
        double s1 = getValue(i1, j0) * (1.0 - u) + getValue(i1, j1) * u;
        return s0 * (1.0 - v) + s1 * v;
    }
    double getMaxValue() const {
        return *std::max_element(std::begin(data), std::end(data));
    }
private:
    PhotometricType type;
    core::vector<double> hAngles;
    core::vector<double> vAngles;
    core::vector<double> data;
};

class CIESProfileParser {
public:
    CIESProfileParser(char* buf, size_t size) {
        ss << std::string(buf, size);
    }

    bool parse(CIESProfile& result) {
        // skip metadata
        std::string line;
        while (std::getline(ss, line)) {
            if (line == "TILT=INCLUDE" || line == "TILT=NONE") break;
        }
        ss.ignore();

        if (line == "TILT=INCLUDE") {
            double lampToLuminaire = getDouble("lampToLuminaire truncated");
            int numTilt = getDouble("numTilt truncated");
            for (int i = 0; i < numTilt; i++)
                getDouble("tilt angle truncated");
            for (int i = 0; i < numTilt; i++)
                getDouble("tilt multiplying factor truncated");
        }
        else if (line != "TILT=NONE") {
            errorMsg = "TILT not specified";
            return false;
        }
        
        int numLamps = getInt("numLamps truncated");
        double lumensPerLamp = getDouble("lumensPerLamp truncated");
        double candelaMultiplier = getDouble("candelaMultiplier truncated");
        int vSize = getInt("vSize truncated");
        int hSize = getInt("hSize truncated");

        int type_ = getInt("type truncated");
        if (error) return false;
        if (type_ <= 0 || type_ > 3) {
            errorMsg = "unrecognized type";
            return false;
        }
        CIESProfile::PhotometricType type = static_cast<CIESProfile::PhotometricType>(type_);
        assert(type == CIESProfile::PhotometricType::TYPE_C && "Only type C is supported for now");

        int unitsType = getInt("unitsType truncated");
        double width = getDouble("width truncated"), length = getDouble("length truncated"), height = getDouble("height truncated");
        double ballastFactor = getDouble("ballastFactor truncated");
        double reserved = getDouble("reserved truncated");
        double inputWatts = getDouble("inputWatts truncated");
        if (error) return false;

        result = CIESProfile(type, hSize, vSize);
        auto& vAngles = result.getVertAngles();
        for (int i = 0; i < vSize; i++) {
            vAngles[i] = getDouble("vertical angle truncated");
            if (i != 0 && vAngles[i - 1] > vAngles[i]) return false; // Angles should be sorted
        }
        assert((vAngles[0] == 0.0 || vAngles[0] == 90.0) && "First angle must be 0 or 90 in type C");
        assert((vAngles[vSize-1] == 90.0 || vAngles[vSize - 1] == 180.0) && "Last angle must be 90 or 180 in type C");

        auto& hAngles = result.getHoriAngles();
        for (int i = 0; i < hSize; i++) {
            hAngles[i] = getDouble("horizontal angle truncated");
            if (i != 0 && hAngles[i - 1] > hAngles[i]) return false; // Angles should be sorted
        }
        assert((hAngles[0] == 0.0) && "First angle must be 0 in type C");
        assert((hAngles[hSize - 1] == 0.0 || hAngles[hSize - 1] == 90.0 || hAngles[hSize - 1] == 180.0 || hAngles[hSize - 1] == 360.0) && 
            "Last angle must be 0, 90, 180 or 360 in type C");

        double factor = ballastFactor*candelaMultiplier;
        for (int i = 0; i < hSize; i++) {
            for (int j = 0; j < vSize; j++) {
                result.setValue(i, j, factor*getDouble("intensity value truncated"));
            }
        }

        return !error;
    }
private:
    int getInt(const char* errorMsg) {
        int in;
        if (ss >> in) 
            return in;
        error = true;
        if (!this->errorMsg)
            this->errorMsg = errorMsg;
        return 0;
    }

    double getDouble(const char* errorMsg) {
        double in;
        if (ss >> in) 
            return in;
        error = true;
        if (!this->errorMsg)
            this->errorMsg = errorMsg;
        return -1.0;
    }

    bool error{ false };
    const char* errorMsg{ nullptr };
    std::stringstream ss;
};

class CIESProfileMetadata final : public asset::IAssetMetadata
{
public:
    CIESProfileMetadata(double maxIntensity) : IAssetMetadata(), maxIntensity(maxIntensity)
    {
    }

    _NBL_STATIC_INLINE_CONSTEXPR const char* LoaderName = "CIESProfileLoader";
    const char* getLoaderName() const override { return LoaderName; }

    double getMaxIntensity() const { return maxIntensity; }
private:
    double maxIntensity;
};

class CIESProfileLoader final : public asset::IAssetLoader {
public:
    _NBL_STATIC_INLINE_CONSTEXPR size_t TEXTURE_WIDTH = 1024;
    _NBL_STATIC_INLINE_CONSTEXPR size_t TEXTURE_HEIGHT = 1024;
    _NBL_STATIC_INLINE_CONSTEXPR double MAX_ANGLE = 360.0;

    //! Check if the file might be loaded by this class
    /** Check might look into the file.
    \param file File handle to check.
    \return True if file seems to be loadable. */
    bool isALoadableFileFormat(io::IReadFile* _file) const override {
        const size_t begginingOfFile = _file->getPos();
        _file->seek(0ull);
        std::string versionBuffer(5, ' ');
        _file->read(versionBuffer.data(), versionBuffer.size());
        _file->seek(begginingOfFile);
        return versionBuffer == "IESNA";
    }

    //! Returns an array of string literals terminated by nullptr
    const char** getAssociatedFileExtensions() const override {
        static const char* extensions[]{ "ies", nullptr };
        return extensions;
    }

    //! Returns the assets loaded by the loader
    /** Bits of the returned value correspond to each IAsset::E_TYPE
    enumeration member, and the return value cannot be 0. */
    uint64_t getSupportedAssetTypesBitfield() const override { 
        return asset::IAsset::ET_IMAGE;
    }

    //! Loads an asset from an opened file, returns nullptr in case of failure.
    asset::SAssetBundle loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override = nullptr, uint32_t _hierarchyLevel = 0u) override {
        if (!_file)
            return {};

        core::vector<char> data(_file->getSize());
        _file->read(data.data(), _file->getSize());
        
        CIESProfileParser parser(data.data(), data.size());

        CIESProfile profile;
        if (!parser.parse(profile))
            return {};

        auto image = createTexture(profile, TEXTURE_WIDTH, TEXTURE_HEIGHT);
        if (!image)
            return {};
 
        auto meta = core::make_smart_refctd_ptr<CIESProfileMetadata>(profile.getMaxValue());
        return asset::SAssetBundle(std::move(image), { std::move(image) });
    }
private:
    core::smart_refctd_ptr<asset::ICPUImage> createTexture(const CIESProfile& profile, size_t width, size_t height) {
        asset::ICPUImage::SCreationParams imgInfo;
        imgInfo.type = asset::ICPUImage::ET_2D;
        imgInfo.extent.width = width;
        imgInfo.extent.height = height;
        imgInfo.extent.depth = 1u;
        imgInfo.mipLevels = 1u;
        imgInfo.arrayLayers = 1u;
        imgInfo.samples = asset::ICPUImage::ESCF_1_BIT;
        imgInfo.flags = static_cast<asset::IImage::E_CREATE_FLAGS>(0u);
        imgInfo.format = asset::EF_R16_UNORM;
        auto outImg = asset::ICPUImage::create(std::move(imgInfo));

        asset::ICPUImage::SBufferCopy region;
        size_t texelBytesz = asset::getTexelOrBlockBytesize(imgInfo.format);
        size_t bufferRowLength = asset::IImageAssetHandlerBase::calcPitchInBlocks(width, texelBytesz);
        region.bufferRowLength = bufferRowLength;
        region.imageExtent = imgInfo.extent;
        region.imageSubresource.baseArrayLayer = 0u;
        region.imageSubresource.layerCount = 1u;
        region.imageSubresource.mipLevel = 0u;
        region.bufferImageHeight = 0u;
        region.bufferOffset = 0u;
        auto buffer = core::make_smart_refctd_ptr<asset::ICPUBuffer>(texelBytesz * bufferRowLength * height);

        double maxValue = profile.getMaxValue();
        double maxValueRecip = 1.0 / maxValue;

        double horiAngleRate = MAX_ANGLE / height;
        double vertAngleRate = MAX_ANGLE / width;
        char* bufferPtr = reinterpret_cast<char*>(buffer->getPointer());
        for (size_t i = 0; i < height; i++) {
            for (size_t j = 0; j < width; j++) {
                double I = profile.sample(i * horiAngleRate, j * vertAngleRate);
                uint16_t value = static_cast<uint16_t>(std::clamp(I * maxValueRecip * 65535.0, 0.0, 65535.0));
                *reinterpret_cast<uint16_t*>(bufferPtr + i * bufferRowLength + j * texelBytesz) = value;
            }
        }

        outImg->setBufferAndRegions(std::move(buffer), core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<asset::IImage::SBufferCopy>>(1ull, region));
        return outImg;
    }
};

}
}
}

#endif