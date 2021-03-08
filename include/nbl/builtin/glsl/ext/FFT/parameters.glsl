// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef _NBL_GLSL_EXT_FFT_PARAMETERS_INCLUDED_
#define _NBL_GLSL_EXT_FFT_PARAMETERS_INCLUDED_

#include "nbl/builtin/glsl/ext/FFT/parameters_struct.glsl"

#ifndef _NBL_GLSL_EXT_FFT_GET_PARAMETERS_DECLARED_
#define _NBL_GLSL_EXT_FFT_GET_PARAMETERS_DECLARED_
nbl_glsl_ext_FFT_Parameters_t nbl_glsl_ext_FFT_getParameters();
#endif

uvec3 nbl_glsl_ext_FFT_Parameters_t_getInputDimensions()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return (params.input_dimensions.xyz);
}

uint nbl_glsl_ext_FFT_Parameters_t_getLog2FFTSize()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return params.input_dimensions.w&0x000000ffu;
}
uint nbl_glsl_ext_FFT_Parameters_t_getDirection()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return (params.input_dimensions.w>>8u)&0x3u;
}
uint nbl_glsl_ext_FFT_Parameters_t_getPaddingType()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return (params.input_dimensions.w>>10u)&0x3u;
}
bool nbl_glsl_ext_FFT_Parameters_t_getIsInverse()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return bool(params.input_dimensions.w&0x00001000u);
}
uint nbl_glsl_ext_FFT_Parameters_t_getNumChannels()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return params.input_dimensions.w>>13u;
}

uvec3 nbl_glsl_ext_FFT_Parameters_t_getInputStrides()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return (params.input_strides.xyz);
}
uvec3 nbl_glsl_ext_FFT_Parameters_t_getOutputStrides()
{
    nbl_glsl_ext_FFT_Parameters_t params = nbl_glsl_ext_FFT_getParameters();
    return (params.output_strides.xyz);
}

#endif