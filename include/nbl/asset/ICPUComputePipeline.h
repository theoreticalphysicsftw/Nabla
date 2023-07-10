// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef __NBL_ASSET_I_CPU_COMPUTE_PIPELINE_H_INCLUDED__
#define __NBL_ASSET_I_CPU_COMPUTE_PIPELINE_H_INCLUDED__

#include "nbl/asset/IComputePipeline.h"
#include "nbl/asset/ICPUPipelineLayout.h"
#include "nbl/asset/ICPUSpecializedShader.h"

namespace nbl
{
namespace asset
{

//! CPU Version of Compute Pipeline
/*
    @see IComputePipeline
*/

class ICPUComputePipeline : public IComputePipeline<ICPUSpecializedShader, ICPUPipelineLayout>, public IAsset
{
    using base_t = IComputePipeline<ICPUSpecializedShader, ICPUPipelineLayout>;

public:
    using base_t::base_t;

    size_t conservativeSizeEstimate() const override { return sizeof(void*)*3u+sizeof(uint8_t); }
  
    core::smart_refctd_ptr<IAsset> clone(uint32_t _depth = ~0u) const override
    {
        core::smart_refctd_ptr<ICPUPipelineLayout> layout = (_depth > 0u && m_layout) ? core::smart_refctd_ptr_static_cast<ICPUPipelineLayout>(m_layout->clone(_depth-1u)) : m_layout;
        core::smart_refctd_ptr<ICPUSpecializedShader> shader = (_depth > 0u && m_shader) ? core::smart_refctd_ptr_static_cast<ICPUSpecializedShader>(m_shader->clone(_depth-1u)) : m_shader;

        auto cp = core::make_smart_refctd_ptr<ICPUComputePipeline>(std::move(layout), std::move(shader));
        clone_common(cp.get());

        return cp;
    }

    _NBL_STATIC_INLINE_CONSTEXPR auto AssetType = ET_COMPUTE_PIPELINE;
    inline E_TYPE getAssetType() const override { return AssetType; }

    ICPUPipelineLayout* getLayout() 
    {
        assert(!isImmutable_debug());
        return m_layout.get(); 
    }
    const ICPUPipelineLayout* getLayout() const { return m_layout.get(); }

    inline void setLayout(core::smart_refctd_ptr<ICPUPipelineLayout>&& _layout)
    {
        assert(!isImmutable_debug());
        m_layout = std::move(_layout);
    }

    ICPUSpecializedShader* getShader()
    {
        assert(!isImmutable_debug());
        return m_shader.get();
    }
    const ICPUSpecializedShader* getShader() const { return m_shader.get(); }
    void setShader(ICPUSpecializedShader* _cs) 
    {
        assert(!isImmutable_debug());
        m_shader = core::smart_refctd_ptr<ICPUSpecializedShader>(_cs); 
    }

protected:

    bool compatible(const IAsset* _other) const override
    {
        //auto* other = static_cast<const ICPUComputePipeline*>(_other);
        return true;
    }

    nbl::core::vector<IAsset*> getMembersToRecurse() const override { return { m_shader.get(), m_layout.get() }; }

    bool isAnyDependencyDummy_impl(uint32_t _levelsBelow) const override
    {
        --_levelsBelow;
        return m_shader->isAnyDependencyDummy(_levelsBelow) || m_layout->isAnyDependencyDummy(_levelsBelow);
    }

    virtual ~ICPUComputePipeline() = default;
};

}
}

#endif