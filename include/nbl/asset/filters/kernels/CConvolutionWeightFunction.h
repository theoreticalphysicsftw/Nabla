// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h
#ifndef _NBL_ASSET_C_CONVOLUTION_IMAGE_FILTER_KERNEL_H_INCLUDED_
#define _NBL_ASSET_C_CONVOLUTION_IMAGE_FILTER_KERNEL_H_INCLUDED_


// TODO: replace with that unified header of basic weight functions
#include "nbl/asset/filters/kernels/CDiracImageFilterKernel.h"
#include "nbl/asset/filters/kernels/CBoxImageFilterKernel.h"
#include "nbl/asset/filters/kernels/CTriangleImageFilterKernel.h"
#include "nbl/asset/filters/kernels/CGaussianImageFilterKernel.h"
#include "nbl/asset/filters/kernels/CKaiserImageFilterKernel.h"
#include "nbl/asset/filters/kernels/CMitchellImageFilterKernel.h"


namespace nbl::asset
{

// this is the horribly slow generic version that you should not use (only use the specializations or when one of the weights is a dirac)
template<typename Weight1DFunctionA, typename Weight1DFunctionB>
class CConvolutionWeightFunction
{
		static_assert(std::is_same_v<impl::weight_function_value_type_t<Weight1DFunctionA>,impl::weight_function_value_type_t<Weight1DFunctionB>>, "Both functions must use the same Value Type!");


		const Weight1DFunctionA m_kernelA;
		const Weight1DFunctionB m_kernelB;
		const float m_ratio;

		std::pair<double,double> getIntegrationDomain(const float x) const
		{
			constexpr float WidthA = Weight1DFunctionA::max_support-Weight1DFunctionA::min_support;
			const float WidthB = (Weight1DFunctionB::max_support-Weight1DFunctionB::min_support)/m_ratio;
			
			// TODO: redo to account for `m_ratio`
			assert(minIntegrationLimit <= maxIntegrationLimit);

			return { minIntegrationLimit, maxIntegrationLimit};
		}
	public:
		constexpr static inline float min_support = Weight1DFunctionA::min_support+Weight1DFunctionB::min_support;
		constexpr static inline float max_support = Weight1DFunctionA::max_support+Weight1DFunctionB::max_support;
		constexpr static inline uint32_t k_smoothness = Weight1DFunctionA::k_smoothness+Weight1DFunctionB::k_smooothness;
	
		// `_ratio` is the width ratio between kernel A and B, our operator() computes `a(x) \conv b(x*_ratio)`
		// if you want to compute `f(x) = a(x/c_1) \conv b(x/c_2)` then you can compute `f(x) = c_1 g(u)` where `u=x/c_1` and `_ratio = c_1/c_2`
		// so `g(u) = a(u) \conv b(u*_ratio) = Integrate[a(u-t)*b(t*_ratio),dt]` and there's no issue with uniform scaling.
		// NOTE: Blit Utils want `f(x) = a(x/c_1)/c_1 \conv b(x/c_2)/c_2` where often `c_1 = 1`
		inline CConvolutionImageFilterKernel(Weight1DFunctionA&& kernelA, Weight1DFunctionB&& kernelB, float _ratio)
			: m_kernelA(std::move(kernelA)), m_kernelB(std::move(kernelB)), m_ratio(_ratio)
		{
			// not equipped to deal with negative scales !
			assert(m_ratio>0.f);
		}

		template<int32_t derivative>
		float operator()(const float x, const uint32_t channel, const uint32_t sampleCount = 64u) const
		{
			if constexpr (std::is_same_v<Weight1DFunctionB,DiracWeight1DFunction>)
				return m_kernelA.operator(x,channel)<derivative>();
			else if (std::is_same_v<Weight1DFunctionA,DiracWeight1DFunction>)
				return m_kernelB.operator(x,channel)<derivative>();
			else
			{
				constexpr auto deriv_A = std::min(Weight1DFunctionA::k_smoothness,derivative);
				constexpr auto deriv_B = derivative-deriv_A;

				auto [minIntegrationLimit, maxIntegrationLimit] = getIntegrationDomain(x);
				// if this happens, it means that `m_ratio=INF` and it degenerated into a dirac delta
				if (minIntegrationLimit == maxIntegrationLimit)
					return m_kernelA.operator()<derivative>(x,channel)*WeightFunctionB::k_energy[channel];

				// if this happened then `m_ratio=0` and we have infinite domain, this is not a problem
				const double dt = (maxIntegrationLimit-minIntegrationLimit)/sampleCount;
				if (core::is_nan<double>(dt))
					return m_kernelA.operator()<deriv_A>(x,channel)*m_kernelB.operator()<deriv_B>(0.f,channel);

				double result = 0.0;
				for (uint32_t i = 0u; i < sampleCount; ++i)
				{
					const double t = minIntegrationLimit + i*dt;
					result += m_kernelA.operator()<deriv_A>(x - t, channel) * m_kernelB.operator()<deriv_B>(t * m_ratio, channel) * dt;
				}
				return static_cast<float>(result);
			}
		}
};

// TODO: redo all to account for `m_ratio`
template <>
float CConvolutionWeightFunction<CBoxImageFilterKernel, CBoxImageFilterKernel>::weight(const float x, const uint32_t channel, const uint32_t) const;

template <>
float CConvolutionWeightFunction<CGaussianImageFilterKernel, CGaussianImageFilterKernel>::weight(const float x, const uint32_t channel, const uint32_t) const;

// TODO: Specialization: CConvolutionImageFilterKernel<Triangle,Triangle> = this is tricky but feasible

template <>
float CConvolutionWeightFunction<CKaiserImageFilterKernel, CKaiserImageFilterKernel>::weight(const float x, const uint32_t channel, const uint32_t) const;

} // end namespace nbl::asset

#endif
