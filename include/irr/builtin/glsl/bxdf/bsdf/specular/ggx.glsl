#ifndef _IRR_BUILTIN_GLSL_BXDF_BSDF_SPECULAR_GGX_INCLUDED_
#define _IRR_BUILTIN_GLSL_BXDF_BSDF_SPECULAR_GGX_INCLUDED_

#include <irr/builtin/glsl/bxdf/common_samples.glsl>
#include <irr/builtin/glsl/bxdf/ndf/ggx.glsl>
#include <irr/builtin/glsl/bxdf/geom/smith/ggx.glsl>
#include <irr/builtin/glsl/bxdf/brdf/specular/ggx.glsl>


float irr_glsl_ggx_height_correlated_aniso_dielectric_cos_eval_wo_clamps(
    in float NdotH2, in float TdotH2, in float BdotH2,
    in float absNdotL, in float NdotL2, in float TdotL2, in float BdotL2,
    in float absNdotV, in float NdotV2, in float TdotV2, in float BdotV2,
    in bool transmitted, in float VdotH, in float LdotH, in float VdotHLdotH,
    in float orientedEta, in float orientedEta2,
    in float ax, in float ax2, in float ay, in float ay2)
{
    float NG_already_in_reflective_dL_measure = irr_glsl_ggx_height_correlated_aniso_cos_eval_DG_wo_clamps(NdotH2,TdotH2,BdotH2,absNdotL,NdotL2,TdotL2,BdotL2,absNdotV,NdotV2,TdotV2,BdotV2,ax,ax2,ay,ay2);
    
    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2,abs(VdotH));

    return reflectance*irr_glsl_ggx_microfacet_to_light_measure_transform(NG_already_in_reflective_dL_measure,absNdotL,transmitted,VdotH,LdotH,VdotHLdotH,orientedEta);
}

// before calling you must ensure that `irr_glsl_AnisotropicMicrofacetCache` is valid (if a given V vector can "see" the L vector)
float irr_glsl_ggx_height_correlated_aniso_dielectric_cos_eval(in irr_glsl_LightSample _sample, in irr_glsl_AnisotropicViewSurfaceInteraction interaction, in irr_glsl_AnisotropicMicrofacetCache _cache, in float eta, in float ax, in float ay)
{
    const float TdotH2 = _cache.TdotH*_cache.TdotH;
    const float BdotH2 = _cache.BdotH*_cache.BdotH;

    const float TdotL2 = _sample.TdotL*_sample.TdotL;
    const float BdotL2 = _sample.BdotL*_sample.BdotL;

    const float TdotV2 = interaction.TdotV*interaction.TdotV;
    const float BdotV2 = interaction.BdotV*interaction.BdotV;

    const float VdotH = _cache.isotropic.VdotH;

    float orientedEta, dummy;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, dummy, VdotH, eta);
    const float orientedEta2 = orientedEta*orientedEta;
    
    const float VdotHLdotH = VdotH*_cache.isotropic.LdotH;
    const bool transmitted = VdotHLdotH<0.0;

    return irr_glsl_ggx_height_correlated_aniso_dielectric_cos_eval_wo_clamps(
        _cache.isotropic.NdotH2,TdotH2,BdotH2,
        abs(_sample.NdotL),_sample.NdotL2,TdotL2,BdotL2,
        abs(interaction.isotropic.NdotV),interaction.isotropic.NdotV_squared,TdotV2,BdotV2,
        transmitted,VdotH,_cache.isotropic.LdotH,VdotHLdotH,orientedEta,orientedEta2,
        ax,ax*ax,ay,ay*ay
    );
}


float irr_glsl_ggx_height_correlated_dielectric_cos_eval_wo_clamps(
    in float NdotH2, in float absNdotL, in float NdotL2,
    in float absNdotV, in float NdotV2,
    in bool transmitted, in float VdotH, in float LdotH, in float VdotHLdotH,
    in float orientedEta, in float orientedEta2, in float a2)
{
    const float NG_already_in_reflective_dL_measure = irr_glsl_ggx_height_correlated_cos_eval_DG_wo_clamps(NdotH2, absNdotL, NdotL2, absNdotV, NdotV2, a2);
    
    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2,abs(VdotH));

    return reflectance*irr_glsl_ggx_microfacet_to_light_measure_transform(NG_already_in_reflective_dL_measure,absNdotL,transmitted,VdotH,LdotH,VdotHLdotH,orientedEta);
}

// before calling you must ensure that `irr_glsl_AnisotropicMicrofacetCache` is valid (if a given V vector can "see" the L vector)
float irr_glsl_ggx_height_correlated_dielectric_cos_eval(in irr_glsl_LightSample _sample, in irr_glsl_IsotropicViewSurfaceInteraction interaction, in irr_glsl_IsotropicMicrofacetCache _cache, in float eta, in float a2)
{
    float orientedEta, dummy;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, dummy, _cache.VdotH, eta);
    const float orientedEta2 = orientedEta*orientedEta;
    
    const float VdotHLdotH = _cache.VdotH*_cache.LdotH;
    const bool transmitted = VdotHLdotH<0.0;

    return irr_glsl_ggx_height_correlated_dielectric_cos_eval_wo_clamps(
        _cache.NdotH2,abs(_sample.NdotL),_sample.NdotL2,
        abs(interaction.NdotV),interaction.NdotV_squared,
        transmitted,_cache.VdotH,_cache.LdotH,VdotHLdotH,orientedEta,orientedEta2,a2
    );
}

// TODO: unifty the two following functions into `irr_glsl_microfacet_BSDF_cos_generate_wo_clamps(vec3 H,...)` and `irr_glsl_microfacet_BSDF_cos_generate` or at least a auto declaration macro in lieu of a template
irr_glsl_LightSample irr_glsl_ggx_dielectric_cos_generate_wo_clamps(in vec3 localV, in bool backside, in vec3 upperHemisphereLocalV, in mat3 m, in vec3 u, in float _ax, in float _ay, in float rcpOrientedEta, in float orientedEta2, in float rcpOrientedEta2, out irr_glsl_AnisotropicMicrofacetCache _cache)
{
    // thanks to this manouvre the H will always be in the upper hemisphere (NdotH>0.0)
    const vec3 H = irr_glsl_ggx_cos_generate(upperHemisphereLocalV,u.xy,_ax,_ay);

    const float VdotH = dot(localV, H);
    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2, abs(VdotH));
    
    float rcpChoiceProb;
    bool transmitted = irr_glsl_partitionRandVariable(reflectance, u.z, rcpChoiceProb);
    
    vec3 localL;
    _cache = irr_glsl_calcAnisotropicMicrofacetCache(transmitted,localV,H,localL,rcpOrientedEta,rcpOrientedEta2);
    
    return irr_glsl_createLightSampleTangentSpace(localV,localL,m);
}

irr_glsl_LightSample irr_glsl_ggx_dielectric_cos_generate(in irr_glsl_AnisotropicViewSurfaceInteraction interaction, in vec3 u, in float ax, in float ay, in float eta, out irr_glsl_AnisotropicMicrofacetCache _cache)
{
    const vec3 localV = irr_glsl_getTangentSpaceV(interaction);
    
    float orientedEta, rcpOrientedEta;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, rcpOrientedEta, interaction.isotropic.NdotV, eta);
    
    const vec3 upperHemisphereV = backside ? (-localV):localV;

    const mat3 m = irr_glsl_getTangentFrame(interaction);
    return irr_glsl_ggx_dielectric_cos_generate_wo_clamps(localV,backside,upperHemisphereV,m, u,ax,ay, rcpOrientedEta,orientedEta*orientedEta,rcpOrientedEta*rcpOrientedEta,_cache);
}



float irr_glsl_ggx_dielectric_pdf_wo_clamps(in bool transmitted, in float reflectance, in float ndf, in float devsh_v, in float absNdotV, in float VdotH, in float LdotH, in float VdotHLdotH, in float orientedEta)
{
    return irr_glsl_smith_VNDF_pdf_wo_clamps(ndf,irr_glsl_GGXSmith_G1_wo_numerator(absNdotV,devsh_v),absNdotV,transmitted,VdotH,LdotH,VdotHLdotH,orientedEta,reflectance);
}
float irr_glsl_ggx_dielectric_pdf_wo_clamps(in bool transmitted, in float reflectance, in float NdotH2, in float absNdotV, in float NdotV2, in float VdotH, in float LdotH, in float VdotHLdotH, in float a2, in float orientedEta)
{
    const float ndf = irr_glsl_ggx_trowbridge_reitz(a2, NdotH2);
    const float devsh_v = irr_glsl_smith_ggx_devsh_part(NdotV2, a2, 1.0-a2);

    return irr_glsl_ggx_dielectric_pdf_wo_clamps(transmitted,reflectance, ndf,devsh_v, absNdotV, VdotH,LdotH,VdotHLdotH, orientedEta);
}

float irr_glsl_ggx_dielectric_pdf_wo_clamps(in bool transmitted, in float reflectance, in float NdotH2, in float TdotH2, in float BdotH2, in float absNdotV, in float NdotV2, in float TdotV2, in float BdotV2, in float VdotH, in float LdotH, in float VdotHLdotH, in float ax, in float ay, in float ax2, in float ay2, in float orientedEta)
{
    const float ndf = irr_glsl_ggx_aniso(TdotH2,BdotH2,NdotH2, ax, ay, ax2, ay2);
    const float devsh_v = irr_glsl_smith_ggx_devsh_part(TdotV2, BdotV2, NdotV2, ax2, ay2);

    return irr_glsl_ggx_dielectric_pdf_wo_clamps(transmitted,reflectance, ndf,devsh_v, absNdotV, VdotH,LdotH,VdotHLdotH, orientedEta);
}

// before calling you must ensure that `irr_glsl_AnisotropicMicrofacetCache` is valid (if a given V vector can "see" the L vector)
float irr_glsl_ggx_height_correlated_dielectric_cos_eval_and_pdf(out float pdf, in irr_glsl_LightSample _sample, in irr_glsl_IsotropicViewSurfaceInteraction interaction, in irr_glsl_IsotropicMicrofacetCache _cache, in float eta, in float a2)
{
    float orientedEta, dummy;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, dummy, _cache.VdotH, eta);
    const float orientedEta2 = orientedEta * orientedEta;

    const float VdotHLdotH = _cache.VdotH * _cache.LdotH;
    const bool transmitted = VdotHLdotH < 0.0;

    const float absNdotV = abs(interaction.NdotV);
    const float NdotV2 = interaction.NdotV_squared;

    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2, abs(_cache.VdotH));

    pdf = irr_glsl_ggx_dielectric_pdf_wo_clamps(transmitted, reflectance, _cache.NdotH2, absNdotV, NdotV2, _cache.VdotH, _cache.LdotH, a2, orientedEta);

    return irr_glsl_ggx_height_correlated_dielectric_cos_eval_wo_clamps(
        _cache.NdotH2, abs(_sample.NdotL), _sample.NdotL2,
        abs(interaction.NdotV), interaction.NdotV_squared,
        transmitted, _cache.VdotH, _cache.LdotH, VdotHLdotH, orientedEta, orientedEta2, a2
    );
}

float irr_glsl_ggx_dielectric_cos_remainder_and_pdf_wo_clamps(out float pdf, in float ndf, in bool transmitted, in float absNdotL, in float NdotL2, in float absNdotV, in float NdotV2, in float VdotH, in float LdotH, in float VdotHLdotH, in float reflectance, in float orientedEta, in float a2)
{
    const float one_minus_a2 = 1.0 - a2;
    const float devsh_v = irr_glsl_smith_ggx_devsh_part(NdotV2, a2, one_minus_a2);
    pdf = irr_glsl_ggx_dielectric_pdf_wo_clamps(transmitted,reflectance, ndf,devsh_v, absNdotV, VdotH,LdotH,VdotHLdotH, orientedEta);

    return irr_glsl_ggx_smith_G2_over_G1_devsh(absNdotL, NdotL2, absNdotV, devsh_v, a2, one_minus_a2);
}

float irr_glsl_ggx_dielectric_cos_remainder_and_pdf(out float pdf, in irr_glsl_LightSample _sample, in irr_glsl_IsotropicViewSurfaceInteraction interaction, in irr_glsl_IsotropicMicrofacetCache _cache, in float eta, in float a2)
{    
    const float ndf = irr_glsl_ggx_trowbridge_reitz(a2, _cache.NdotH2);
    
    float orientedEta, dummy;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, dummy, _cache.VdotH, eta);
    const float orientedEta2 = orientedEta*orientedEta;

    const float VdotHLdotH = _cache.VdotH*_cache.LdotH;
    const bool transmitted = VdotHLdotH<0.0;
    
    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2,abs(_cache.VdotH));

    const float absNdotV = abs(interaction.NdotV);
    return irr_glsl_ggx_dielectric_cos_remainder_and_pdf_wo_clamps(pdf,ndf,transmitted, abs(_sample.NdotL),_sample.NdotL2, absNdotV,interaction.NdotV_squared, _cache.VdotH,_cache.LdotH,VdotHLdotH, reflectance,orientedEta, a2);
}


float irr_glsl_ggx_aniso_dielectric_cos_remainder_and_pdf_wo_clamps(out float pdf, in float ndf, in bool transmitted, in float absNdotL, in float NdotL2, in float TdotL2, in float BdotL2, in float absNdotV, in float TdotV2, in float BdotV2, in float NdotV2, in float VdotH, in float LdotH, in float VdotHLdotH, in float reflectance, in float orientedEta, in float ax2,in float ay2)
{
    const float devsh_v = irr_glsl_smith_ggx_devsh_part(TdotV2, BdotV2, NdotV2, ax2, ay2);
    pdf = irr_glsl_ggx_dielectric_pdf_wo_clamps(transmitted,reflectance, ndf,devsh_v, absNdotV, VdotH,LdotH,VdotHLdotH, orientedEta);

    return irr_glsl_ggx_smith_G2_over_G1_devsh(
        absNdotL, TdotL2,BdotL2,NdotL2,
        absNdotV, devsh_v,
        ax2, ay2
    );
}

float irr_glsl_ggx_aniso_dielectric_cos_remainder_and_pdf(out float pdf, in irr_glsl_LightSample _sample, in irr_glsl_AnisotropicViewSurfaceInteraction interaction, in irr_glsl_AnisotropicMicrofacetCache _cache, in float eta, in float ax, in float ay)
{
    const float ax2 = ax*ax;
    const float ay2 = ay*ay;
    const float TdotH2 = _cache.TdotH*_cache.TdotH;
    const float BdotH2 = _cache.BdotH*_cache.BdotH;
    const float ndf = irr_glsl_ggx_aniso(TdotH2,BdotH2,_cache.isotropic.NdotH2, ax, ay, ax2, ay2);

    const float TdotL2 = _sample.TdotL*_sample.TdotL;
    const float BdotL2 = _sample.BdotL*_sample.BdotL;

    const float TdotV2 = interaction.TdotV*interaction.TdotV;
    const float BdotV2 = interaction.BdotV*interaction.BdotV;

    const float VdotH = _cache.isotropic.VdotH;

    float orientedEta, dummy;
    const bool backside = irr_glsl_getOrientedEtas(orientedEta, dummy, VdotH, eta);
    const float orientedEta2 = orientedEta*orientedEta;
    
    const float VdotHLdotH = VdotH*_cache.isotropic.LdotH;
    const bool transmitted = VdotHLdotH<0.0;
    
    const float reflectance = irr_glsl_fresnel_dielectric_common(orientedEta2,abs(VdotH));

    const float absNdotV = abs(interaction.isotropic.NdotV);
	return irr_glsl_ggx_aniso_dielectric_cos_remainder_and_pdf_wo_clamps(pdf,ndf,transmitted, abs(_sample.NdotL),_sample.NdotL2,TdotL2,BdotL2, absNdotV,TdotV2,BdotV2,interaction.isotropic.NdotV_squared, VdotH,_cache.isotropic.LdotH,VdotHLdotH, reflectance,orientedEta, ax2,ay2);
}

#endif
