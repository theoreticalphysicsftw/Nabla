// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#include <nbl/ext/StreamingDrawing/StreamingDrawing.h>

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

StreamingDrawingBuffer::StreamingDrawingBuffer(
    core::smart_refctd_ptr<video::ILogicalDevice>&& device,
    core::smart_refctd_ptr<video::IGPUFence>&& fence,
    video::IGPUQueue* queue,
    core::smart_refctd_ptr<video::IGPUCommandBuffer>&& cmdbuf,
    uint32_t bufferSize
) : m_device(std::move(device)), m_fence(std::move(fence)), m_queue(queue), m_cmdbuf(std::move(cmdbuf))
{
    const auto& limits = m_device->getPhysicalDevice()->getLimits();
    m_alignment =
        std::max<uint64_t>(
            std::max<uint64_t>(limits.bufferViewAlignment, limits.minSSBOAlignment),
            std::max<uint64_t>(limits.minUBOAlignment, _NBL_SIMD_ALIGNMENT)
        );

    m_alloc = allocator_t(nullptr, 0u, 0u, m_alignment, bufferSize);

    video::IGPUBuffer::SCreationParams params = {};
    params.size = bufferSize;
    params.usage = asset::IBuffer::EUF_STORAGE_BUFFER_BIT;

    m_buffer = m_device->createBuffer(std::move(params));
    auto mreqs = m_buffer->getMemoryReqs();
    mreqs.memoryTypeBits &= m_device->getPhysicalDevice()->getDeviceLocalMemoryTypeBits();
    auto gpubufMem = m_device->allocate(mreqs, m_buffer.get());

    m_cmdbuf->begin(video::IGPUCommandBuffer::EU_NONE);
}

void StreamingDrawingBuffer::drawLines(
    Line* linesBegin,
    Line* linesEnd
)
{
    Line* line = linesBegin;
    while (line != linesEnd)
    {
        allocator_t::size_type allocated = m_alloc.alloc_addr(sizeof(Line), m_alignment);
        if (allocated == allocator_t::invalid_address)
        {
            flush();
            // Try allocating for this line again
            continue;
        }

        // [TODO] Have actual drawing of the line here
        m_cmdbuf->draw(2, 1, 0, 0);

        line++;
    }
}

void StreamingDrawingBuffer::flush() {
    m_cmdbuf->end();

    nbl::video::IGPUQueue::SSubmitInfo submit;
    {
        submit.commandBufferCount = 1u;
        submit.commandBuffers = &m_cmdbuf.get();
        submit.signalSemaphoreCount = 0u;
        submit.waitSemaphoreCount = 0u;

        m_queue->submit(1u, &submit, m_fence.get());
    }

    m_device->blockForFences(1u, &m_fence.get());

    m_alloc.reset();
    m_cmdbuf->begin(video::IGPUCommandBuffer::EU_NONE);
}

StreamingDrawingBuffer::~StreamingDrawingBuffer() {
    flush();
}

}
}
}

