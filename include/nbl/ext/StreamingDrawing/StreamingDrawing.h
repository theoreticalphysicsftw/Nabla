// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef _NBL_EXT_TEXT_RENDERING_H_INCLUDED_
#define _NBL_EXT_TEXT_RENDERING_H_INCLUDED_

#include "nabla.h"

using namespace nbl;
using namespace nbl::core;
using namespace nbl::asset;
using namespace nbl::video;

namespace nbl
{
namespace ext
{
namespace StreamingDrawing
{

struct Line {
	core::vector3df start;
	core::vector3df end;
};

class NBL_API StreamingDrawingBuffer
{
private:
	using allocator_t = core::LinearAddressAllocator<uint64_t>;

	allocator_t m_alloc;
	core::smart_refctd_ptr<video::IGPUBuffer> m_buffer;

	core::smart_refctd_ptr<video::ILogicalDevice> m_device;
	core::smart_refctd_ptr<video::IGPUFence> m_fence;
	video::IGPUQueue* m_queue;
	core::smart_refctd_ptr<video::IGPUCommandBuffer> m_cmdbuf;
	uint64_t m_alignment;
public:
	StreamingDrawingBuffer(
		core::smart_refctd_ptr<video::ILogicalDevice>&& device,
		core::smart_refctd_ptr<video::IGPUFence>&& fence, // unsignalled fence
		video::IGPUQueue* queue, // graphics queue
		core::smart_refctd_ptr<video::IGPUCommandBuffer>&& cmdbuf, // resettable command buffer
		uint32_t bufferSize
	);

	~StreamingDrawingBuffer();

	void drawLines(
		Line* linesBegin,
		Line* linesEnd
	);

	void flush();
};

}
}
}

#endif