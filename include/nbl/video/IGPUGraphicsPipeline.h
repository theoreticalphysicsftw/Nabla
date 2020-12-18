#ifndef __NBL_I_GPU_GRAPHICS_PIPELINE_H_INCLUDED__
#define __NBL_I_GPU_GRAPHICS_PIPELINE_H_INCLUDED__

#include "nbl/asset/IGraphicsPipeline.h"
#include "nbl/video/IGPURenderpass.h"
#include "nbl/video/IGPURenderpassIndependentPipeline.h"

namespace nbl {
namespace video
{

class IGPUGraphicsPipeline : public core::IReferenceCounted, public asset::IGraphicsPipeline<IGPURenderpassIndependentPipeline, IGPURenderpass>
{
    using base_t = asset::IGraphicsPipeline<IGPURenderpassIndependentPipeline, IGPURenderpass>;

protected:
    ~IGPUGraphicsPipeline() = default;

public:
    using base_t::base_t;
};

}
}

#endif