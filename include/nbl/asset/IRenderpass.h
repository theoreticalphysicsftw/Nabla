#ifndef _NBL_I_RENDERPASS_H_INCLUDED_
#define _NBL_I_RENDERPASS_H_INCLUDED_

#include "nbl/core/SRange.h"
#include "nbl/core/containers/refctd_dynamic_array.h"
#include "nbl/core/math/glslFunctions.tcc"

#include "nbl/asset/IImage.h"
#include "nbl/asset/ECommonEnums.h"

#include <compare>

namespace nbl::asset
{

// TODO: move this struct
struct SDepthStencilLayout
{
    IImage::E_LAYOUT depth = IImage::EL_UNDEFINED;
    // if you leave `stencilLayout` as undefined this means you want same layout as `depth`
    IImage::E_LAYOUT stencil = IImage::EL_UNDEFINED;

    auto operator<=>(const SDepthStencilLayout&) const = default;

    inline IImage::E_LAYOUT actualStencilLayout() const
    {
        if (stencil!=IImage::EL_UNDEFINED)
            return stencil;
        // synchronization2 will save us doing these if-checks
        if (depth==IImage::EL_DEPTH_ATTACHMENT_OPTIMAL)
            return IImage::EL_STENCIL_ATTACHMENT_OPTIMAL;
        if (depth==IImage::EL_DEPTH_READ_ONLY_OPTIMAL)
            return IImage::EL_STENCIL_READ_ONLY_OPTIMAL;
        return depth;
    }
};

class IRenderpass
{
    public:
        // TODO: move this out somewhere? looks useful
        template<typename T, class func_t>
        static inline void visitTokenTerminatedArray(const T* array, const T& endToken, func_t& func)
        {
            if (array)
            for (auto it=array; *it!=endToken && func(*it); it++)
            {
            }
        }

        enum class E_LOAD_OP : uint8_t
        {
            LOAD = 0,
            CLEAR,
            DONT_CARE,
            UNDEFINED
        };
        enum class E_STORE_OP: uint8_t
        {
            STORE = 0,
            DONT_CARE,
            UNDEFINED
        };
        enum class E_SUBPASS_DESCRIPTION_FLAGS : uint8_t
        {
            NONE = 0x00,
            PER_VIEW_ATTRIBUTES_BIT = 0x01,
            PER_VIEW_POSITION_X_ONLY_BIT = 0x02
        };
        // For all arrays here we use ArrayNameEnd terminator instead of specifying the count
        struct SCreationParams
        {
            public:
                // funny little struct to allow us to use ops as `layout.depth`, `layout.stencil` and `layout`
                struct Layout : SDepthStencilLayout
                {
                    inline operator IImage::E_LAYOUT() const { return depth; }

                    auto operator<=>(const Layout&) const = default;
                };
                // The reason we don't have separate types per depthstencil, color and resolve is because
                // attachments limits (1 depth, MaxColorAttachments color and resolve) only apply PER SUBPASS
                // so overall we can have as many attachments of whatever type and in whatever order we want
                struct SAttachmentDescription
                {
                    // similar idea for load/store ops as for layouts
                    template<typename op_t>
                    struct Op
                    {
                        op_t depth : 2 = op_t::DONT_CARE;
                        op_t stencil : 2 = op_t::UNDEFINED;


                        auto operator<=>(const Op&) const = default;

                        inline operator op_t() const { return SDepthStencilOp<op_t>::depth; }
                    
                        inline op_t actualStencilOp() const
                        {
                            return stencil!=op_t::UNDEFINED ? stencil:depth;
                        }
                    };

                    E_FORMAT format = EF_UNKNOWN;
                    IImage::E_SAMPLE_COUNT_FLAGS samples : 6 = IImage::ESCF_1_BIT;
                    uint8_t mayAlias : 1 = false;
                    Op<E_LOAD_OP> loadOp = {};
                    Op<E_STORE_OP> storeOp = {};
                    Layout initialLayout = {};
                    Layout finalLayout = {};

                    auto operator<=>(const SAttachmentDescription&) const = default;

                    inline bool valid() const
                    {
                        auto disallowedFinalLayout = [](const auto& layout)->bool
                        {
                            switch (layout)
                            {
                                case IImage::EL_UNDEFINED: [[fallthrough]];
                                case IImage::EL_PREINITIALIZED:
                                    return true;
                                    break;
                                default:
                                    break;
                            }
                            return false;
                        };
                        //
                        if (disallowedFinalLayout(finalLayout))
                            return false;
                        //
                        if (asset::isDepthOrStencilFormat(format) && !asset::isDepthOnlyFormat(format) && disallowedFinalLayout(finalLayout.actualStencilLayout()))
                            return false;
                        return true;
                    }
                    //inline bool used() const {return format!=EF_UNKNOWN;}
                };
                constexpr static inline SAttachmentDescription AttachmentsEnd = {};
                const SAttachmentDescription* attachments = &AttachmentsEnd;
            

                struct SSubpassDescription
                {
                    constexpr static inline uint32_t AttachmentUnused = 0xffFFffFFu;
                    template<typename layout_t>
                    struct SAttachmentRef
                    {
                        public:
                            // If you leave the `attachmentIndex` as default then it means its not being used
                            uint32_t attachmentIndex = AttachmentUnused;
                            layout_t layout = {};

                            auto operator<=>(const SAttachmentRef<layout_t>&) const = default;
                        
                        protected:
                            inline bool invalidLayout(const IImage::E_LAYOUT _layout)
                            {
                                switch (_layout)
                                {
                                    case IImage::EL_UNDEFINED: [[fallthrough]];
                                    case IImage::EL_PREINITIALIZED: [[fallthrough]];
                                    case IImage::EL_PRESENT_SRC:
                                        return true;
                                        break;
                                    default:
                                        break;
                                }
                                return false;
                            }
                            inline bool invalid() const
                            {
                                if (attachmentIndex!=AttachmentUnused)
                                {
                                    //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAttachmentReference2.html#VUID-VkAttachmentReference2-layout-03077
                                    const IImage::E_LAYOUT usageAllOrJustDepth = layout;
                                    if (invalidLayout(usageAllOrJustDepth))
                                        return true;
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAttachmentReferenceStencilLayout.html#VUID-VkAttachmentReferenceStencilLayout-stencilLayout-03318
                                    if constexpr (!std::is_base_of_v<SDepthStencilLayout,layout_t>)
                                    if (invalidLayout(layout.actualStencilLayout())
                                        return true;
                                }
                                return true;
                            }
                    };
                    struct SInputAttachmentRef : SAttachmentRef<Layout>
                    {
                        core::bitflag<IImage::E_ASPECT_FLAGS> aspectMask = IImage::E_ASPECT_FLAGS::EAF_NONE;

                        auto operator<=>(const SInputAttachmentRef&) const = default;

                        inline bool valid() const
                        {
                            if (invalid() || !aspectMask.value)
                                return false;
                            if (attachmentIndex!=AttachmentUnused)
                            {
                                // TODO: synchronization2 will wholly replace this with https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06921
                                switch (layout)
                                {
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06912
                                    case IImage::EL_COLOR_ATTACHMENT_OPTIMAL: [[fallthrough]];
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06918
                                    case IImage::EL_DEPTH_ATTACHMENT_OPTIMAL: [[fallthrough]];
                                    case IImage::EL_STENCIL_ATTACHMENT_OPTIMAL:
                                        return false;
                                        break;
                                    default:
                                        break;
                                }
                            }
                            return true;
                        }
                    };
                    template<typename layout_t>
                    struct SRenderAttachmentRef
                    {
                        constexpr static inline bool IsDepth = std::is_same_v<layout_t,SDepthStencilLayout>;

                        SAttachmentRef<layout_t> render;
                        SAttachmentRef<layout_t> resolve;

                        auto operator<=>(const SRenderAttachmentRef<layout_t>&) const = default;

                        inline bool valid() const
                        {
                            if (render.invalid() || resolve.invalid())
                                return false;
                            const bool renderUsed = render.attachmentIndex!=AttachmentUnused;
                            const bool resolveUsed = resolve.attachmentIndex!=AttachmentUnused;
                            if (renderUsed)
                            {
                                // TODO: synchronization2 will replace all this by just 2 things
                                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06922
                                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06923
                                switch (render.layout)
                                {
                                    case IImage::EL_COLOR_ATTACHMENT_OPTIMAL:
                                        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06915
                                        if constexpr(IsDepth)
                                            return false;
                                        break;
                                    case IImage::EL_SHADER_READ_ONLY_OPTIMAL:
                                        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06913
                                        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06914
                                        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06915
                                        return false;
                                        break;
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06919
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06920
                                    case IImage::EL_DEPTH_ATTACHMENT_OPTIMAL: [[fallthrough]];
                                    case IImage::EL_DEPTH_READ_ONLY_OPTIMAL:
                                        if constexpr (!IsDepth)
                                            return false;
                                        break;
                                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-attachment-06251
                                    case IImage::EL_STENCIL_ATTACHMENT_OPTIMAL: [[fallthrough]];
                                    case IImage::EL_STENCIL_READ_ONLY_OPTIMAL:
                                        return false;
                                        break;
                                    default:
                                        break;
                                }
                                /*
                                if constexpr (IsDepth)
                                switch (render.layout.actualStencilLayout())
                                {
                                        return false;
                                        break;
                                    default:
                                        break;
                                }*/
                            }
                            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-pResolveAttachments-03065
                            else if (resolveUsed)
                                return false;
                            return true;
                        }
                    };
                    using SDepthStencilAttachmentRef = SRenderAttachmentRef<SDepthStencilLayout>;
                    using SColorAttachmentRef = SRenderAttachmentRef<IImage::E_LAYOUT>;


                    auto operator<=>(const SSubpassDescription&) const = default;

                    inline bool valid() const
                    {
                        for (auto i=0u; i<MaxColorAttachments; i++)
                        if (!colorAttachments[i].valid())
                            return false;
                        if (!depthStencilAttachment.valid())
                            return false;
                        bool invalid = false;
                        auto attachmentValidVisitor = [&invalid](const auto& ref)->bool
                        {
                            if (!ref.valid())
                            {
                                invalid = true;
                                return false;
                            }
                            return true;
                        };
                        visitTokenTerminatedArray(inputAttachments,InputAttachmentsEnd,attachmentValidVisitor);
                        if (invalid)
                            return false;
                        return true;
                    }


                    //! Field ordering prioritizes ergonomics
                    static inline constexpr auto MaxColorAttachments = 8u;
                    SColorAttachmentRef colorAttachments[MaxColorAttachments] = {};

                    SDepthStencilAttachmentRef depthStencilAttachment = {};

                    constexpr static inline SInputAttachmentRef InputAttachmentsEnd = {};
                    const SInputAttachmentRef* inputAttachments = &InputAttachmentsEnd;

                    // The arrays pointed to by this array must be terminated by `AttachmentUnused` value
                    const uint32_t* preserveAttachments = &AttachmentUnused;

                    // TODO: shading rate attachment

                    uint32_t viewMask = 0u;
                    E_SUBPASS_DESCRIPTION_FLAGS flags : 3 = E_SUBPASS_DESCRIPTION_FLAGS::NONE;
                    // Do not expose because we don't support Subpass Shading
                    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-pipelineBindPoint-04953
                    //E_PIPELINE_BIND_POINT pipelineBindPoint : 2 = EPBP_GRAPHICS;
                };
                constexpr static inline SSubpassDescription SubpassesEnd = {};
                const SSubpassDescription* subpasses = &SubpassesEnd;

                struct SSubpassDependency
                {
                    uint32_t srcSubpass;
                    uint32_t dstSubpass;
                    E_PIPELINE_STAGE_FLAGS srcStageMask;
                    E_PIPELINE_STAGE_FLAGS dstStageMask;
                    E_ACCESS_FLAGS srcAccessMask;
                    E_ACCESS_FLAGS dstAccessMask;
                    E_DEPENDENCY_FLAGS dependencyFlags;

                    auto operator<=>(const SSubpassDependency&) const = default;
                };
                constexpr static inline SSubpassDependency DependenciesEnd = {};
                const SSubpassDependency* dependencies = &DependenciesEnd;


                // we do this differently than Vulkan so we're not braindead
                static inline constexpr auto MaxMultiviewViewCount = 32u;
                uint8_t viewCorrelationGroup[MaxMultiviewViewCount] = {
                    vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,
                    vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,
                    vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,
                    vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init,vcg_init
                };

            private:
                static inline constexpr auto vcg_init = MaxMultiviewViewCount;
        };

        struct CreationParamValidationResult
        {
            uint32_t attachmentCount = 0u;
            uint32_t subpassCount = 0u;
            uint32_t dependencyCount = 0u;

            inline operator bool() const {return subpassCount;}
        };
        inline virtual CreationParamValidationResult validateCreationParams(const SCreationParams& params)
        {
            CreationParamValidationResult retval = {};
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo2-pSubpasses-parameter
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo2-subpassCount-arraylength
            if (!params.subpasses || params.subpasses[0]==SCreationParams::SubpassesEnd)
                return retval;

            if (params.depthStencilAttachment.used())
            {
                auto disallowedStencilLayouts = [](const IImage::E_LAYOUT layout) -> bool
                {
                    switch (layout)
                    {
                        case IImage::EL_COLOR_ATTACHMENT_OPTIMAL: [[fallthrough]];
                        case IImage::EL_DEPTH_ATTACHMENT_OPTIMAL: [[fallthrough]];
                        case IImage::EL_DEPTH_READ_ONLY_OPTIMAL:
                            return true;
                            break;
                        default:
                            break;
                    }
                    return false;
                };
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkAttachmentDescriptionStencilLayout-stencilInitialLayout-03308
                if (disallowedStencilLayouts(params.depthStencilAttachment.initialLayout.actualStencilLayout()))
                    return false;
                const auto stencilFinalLayout = params.depthStencilAttachment.finalLayout.actualStencilLayout();
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkAttachmentDescriptionStencilLayout-stencilFinalLayout-03309
                if (disallowedStencilLayouts(stencilFinalLayout))
                    return false;
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkAttachmentDescriptionStencilLayout-stencilFinalLayout-03310
                if (disallowedFinalLayouts(stencilFinalLayout))
                    return false;
            }

            //for (auto pSubpass=params.subpasses; *pSubpass!=SCreationParams::SubpassesEnd; pSubpass++)
            visitTokenTerminatedArray(params.subpasses,SCreationParams::SubpassesEnd,[&params](const SCreationParams::SSubpassDescription& subpass)->bool
            {
                // can't validate without allocating unbounded additional memory
                https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-loadOp-03064
    #if 0
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDescription2.html#VUID-VkSubpassDescription2-pResolveAttachments-03066
                auto validateResolve = [subpass]() -> bool
                {
                    if (subpass.)
                }
    #endif
                // TODO: validate references
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo2-attachment-03051
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo2-pSubpasses-06473
                return true;
            });
        
            visitTokenTerminatedArray(params.dependencies,SCreationParams::DependenciesEnd,[&params](const SCreationParams::SSubpassDependency& dependency)->bool
            {
                return true;
            });

            return true;
        }

    explicit IRenderpass(const SCreationParams& params, const uint32_t attachmentCount, const uint32_t subpassCount, const uint32_t dependencyCount) :
        m_params(params),
        m_attachments(attachmentCount ? core::make_refctd_dynamic_array<attachments_array_t>(attachmentCount):nullptr),
        m_subpasses(subpassCount ? core::make_refctd_dynamic_array<subpasses_array_t>(subpassCount):nullptr),
        m_dependencies(dependencyCount ? core::make_refctd_dynamic_array<subpass_deps_array_t>(dependencyCount):nullptr)
    {
        auto attachments = core::SRange<const SCreationParams::SAttachmentDescription>{params.attachments, params.attachments+params.attachmentCount};
        std::copy(attachments.begin(), attachments.end(), m_attachments->begin());
        m_params.attachments = m_attachments->data();

        auto subpasses = core::SRange<const SCreationParams::SSubpassDescription>{params.subpasses, params.subpasses+params.subpassCount};
        std::copy(subpasses.begin(), subpasses.end(), m_subpasses->begin());
        m_params.subpasses = m_subpasses->data();

        uint32_t attRefCnt = 0u;
        uint32_t preservedAttRefCnt = 0u;
        for (const auto& sb : (*m_subpasses))
        {
            attRefCnt += sb.colorAttachmentCount;
            attRefCnt += sb.inputAttachmentCount;
            if (sb.resolveAttachments)
                attRefCnt += sb.colorAttachmentCount;
            if (sb.depthStencilAttachment.attachment!=ATTACHMENT_UNUSED)
                ++attRefCnt;

            if (sb.preserveAttachments)
                preservedAttRefCnt += sb.preserveAttachmentCount;
        }
        if (attRefCnt)
            m_attachmentRefs = core::make_refctd_dynamic_array<attachment_refs_array_t>(attRefCnt);
        if (preservedAttRefCnt)
            m_preservedAttachmentRefs = core::make_refctd_dynamic_array<preserved_attachment_refs_array_t>(preservedAttRefCnt);

        uint32_t refOffset = 0u;
        uint32_t preservedRefOffset = 0u;
        auto* refs = m_attachmentRefs->data();
        auto* preservedRefs = m_preservedAttachmentRefs->data();
        for (auto& sb : (*m_subpasses))
        {
            if (m_attachmentRefs)
            {
#define _COPY_ATTACHMENT_REFS(_array,_count)\
                std::copy(sb._array, sb._array+sb._count, refs+refOffset);\
                sb._array = refs+refOffset;\
                refOffset += sb._count;

                _COPY_ATTACHMENT_REFS(colorAttachments, colorAttachmentCount);
                if (sb.inputAttachments)
                {
                    _COPY_ATTACHMENT_REFS(inputAttachments, inputAttachmentCount);
                }
                if (sb.resolveAttachments)
                {
                    _COPY_ATTACHMENT_REFS(resolveAttachments, colorAttachmentCount);
                }
                if (sb.depthStencilAttachment)
                {
                    refs[refOffset] = sb.depthStencilAttachment[0];
                    sb.depthStencilAttachment = refs + refOffset;
                    ++refOffset;
                }
#undef _COPY_ATTACHMENT_REFS
            }

            if (m_preservedAttachmentRefs)
            {
                std::copy(sb.preserveAttachments, sb.preserveAttachments+sb.preserveAttachmentCount, preservedRefs+preservedRefOffset);
                sb.preserveAttachments = preservedRefs+preservedRefOffset;
                preservedRefOffset += sb.preserveAttachmentCount;
            }
        }

        if (!params.dependencies)
            return;

        auto deps = core::SRange<const SCreationParams::SSubpassDependency>{params.dependencies, params.dependencies+params.dependencyCount};
        std::copy(deps.begin(), deps.end(), m_dependencies->begin());
        m_params.dependencies = m_dependencies->data();
    }

    inline core::SRange<const SCreationParams::SAttachmentDescription> getAttachments() const
    {
        if (!m_attachments)
            return { nullptr, nullptr };
        return { m_attachments->cbegin(), m_attachments->cend() };
    }

    inline core::SRange<const SCreationParams::SSubpassDescription> getSubpasses() const
    {
        if (!m_subpasses)
            return { nullptr, nullptr };
        return { m_subpasses->cbegin(), m_subpasses->cend() };
    }

    inline core::SRange<const SCreationParams::SSubpassDependency> getSubpassDependencies() const
    {
        if (!m_dependencies)
            return { nullptr, nullptr };
        return { m_dependencies->cbegin(), m_dependencies->cend() };
    }

    const SCreationParams& getCreationParameters() const { return m_params; }

protected:
    virtual ~IRenderpass() {}

    SCreationParams m_params;
    using attachments_array_t = core::smart_refctd_dynamic_array<SCreationParams::SAttachmentDescription>;
    // storage for m_params.attachments
    attachments_array_t m_attachments;
    using subpasses_array_t = core::smart_refctd_dynamic_array<SCreationParams::SSubpassDescription>;
    // storage for m_params.subpasses
    subpasses_array_t m_subpasses;
    using subpass_deps_array_t = core::smart_refctd_dynamic_array<SCreationParams::SSubpassDependency>;
    // storage for m_params.dependencies
    subpass_deps_array_t m_dependencies;
    using attachment_refs_array_t = core::smart_refctd_dynamic_array<SCreationParams::SSubpassDescription::SAttachmentRef>;
    attachment_refs_array_t m_attachmentRefs;
    using preserved_attachment_refs_array_t = core::smart_refctd_dynamic_array<uint32_t>;
    preserved_attachment_refs_array_t m_preservedAttachmentRefs;
};

}

#endif
