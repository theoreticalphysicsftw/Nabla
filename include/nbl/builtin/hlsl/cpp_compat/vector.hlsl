
#ifndef _NBL_BUILTIN_HLSL_VECTOR_INCLUDED_
#define _NBL_BUILTIN_HLSL_VECTOR_INCLUDED_

namespace nbl::hlsl
{

#ifndef __HLSL_VERSION 

template<typename T, uint16_t N>
using vector = glm::vec<N, T>;

using float4 = vector<float, 4>;
using float3 = vector<float, 3>;
using float2 = vector<float, 2>;
using float1 = vector<float, 1>;

using int32_t4 = vector<int32_t, 4>;
using int32_t3 = vector<int32_t, 3>;
using int32_t2 = vector<int32_t, 2>;
using int32_t1 = vector<int32_t, 1>;

using uint32_t4 = vector<uint32_t, 4>;
using uint32_t3 = vector<uint32_t, 3>;
using uint32_t2 = vector<uint32_t, 2>;
using uint32_t1 = vector<uint32_t, 1>;


#endif

}

#endif