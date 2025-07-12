#version 460 core

#include "Include.Pi.glsl"
const float Epsilon = 0.00001;
const uint NumSamples = 1024;
const float InvNumSamples = 1.0 / float(NumSamples);
const int NumMipLevels = 1;

layout(binding = 0) uniform samplerCube s_environment;
layout(binding = 0, rgba16f) restrict writeonly uniform imageCube s_prefiltered;

layout(location = 0) uniform float u_roughness;

#include "Include.Hammersley.glsl"

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float Random(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
vec2 SampleHammersley(uint i) {
    return vec2(i * InvNumSamples, RadicalInverse_VdC(i));
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
vec3 sampleGGX(float u1, float u2, float roughness) {
    float alpha = roughness * roughness;

    float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha * alpha - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
    float phi = TwoPI * u1;

    // Convert to Cartesian upon return.
    return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness) {
    float alpha   = roughness * roughness;
    float alphaSquared = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSquared - 1.0) + 1.0;
    return alphaSquared / (PI * denom * denom);
}

// Calculate normalized sampling direction vector based on current fragment coordinates (gl_GlobalInvocationID.xyz).
// This is essentially "inverse-sampling": we reconstruct what the sampling vector would be if we wanted it to "hit"
// this particular fragment in a cubemap.
// See: OpenGL core profile specs, section 8.13.
vec3 GetSamplingVector(vec2 imageDimensions) {
    vec2 st = gl_GlobalInvocationID.xy / imageDimensions;
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - vec2(1.0);

    vec3 ret;
    // Sadly 'switch' doesn't seem to work, at least on NVIDIA.
    if(gl_GlobalInvocationID.z == 0)      ret = vec3(1.0,  uv.y, -uv.x);
    else if(gl_GlobalInvocationID.z == 1) ret = vec3(-1.0, uv.y,  uv.x);
    else if(gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0, -uv.y);
    else if(gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0, uv.y);
    else if(gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0);
    else if(gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0);
    return normalize(ret);
}

void ComputeBasisVectors(const vec3 N, out vec3 S, out vec3 T) {
    // Branchless select non-degenerate T.
    T = cross(N, vec3(0.0, 1.0, 0.0));
    T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

    T = normalize(T);
    S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
vec3 TangentToWorld(const vec3 v, const vec3 N, const vec3 S, const vec3 T) {
    return S * v.x + T * v.y + N * v.z;
}

vec3 SampleHemisphere(float u1, float u2) {
    const float u1p = sqrt(max(0.0, 1.0 - u1 * u1));
    return vec3(cos(TwoPI * u2) * u1p, sin(TwoPI * u2) * u1p, u1);
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(void)
{
    // Make sure we won't write past output when computing higher mipmap levels.
    ivec2 outputSize = imageSize(s_prefiltered);
    if(gl_GlobalInvocationID.x >= outputSize.x || gl_GlobalInvocationID.y >= outputSize.y)
    {
        return;
    }

    // Solid angle associated with a single cubemap texel at zero mipmap level.
    // This will come in handy for importance sampling below.
    float envMapDim = float(textureSize(s_environment, 0).s);
    float wt = 4.0 * PI / (6 * envMapDim * envMapDim);

    vec3 color = vec3(0);
    float totalWeight = 0;
    vec3 N = GetSamplingVector(outputSize);
    vec3 Lo = N;
    vec3 S, T;
    ComputeBasisVectors(N, S, T);
    // Convolve environment map using GGX NDF importance sampling.
    // Weight by cosine term since Epic claims it generally improves quality.

    float roughness = clamp(u_roughness, 0.005, 0.995);
    float mipLevel = max(0.5 * log2((1.0 / NumSamples) / wt) + 1.0, 0.0);
    if (mipLevel == 0.0)
    {
        color += textureLod(s_environment, N, 0).rgb;
    }
    else
    {
        float weight = 0;

        // Convolve environment map using GGX NDF importance sampling.
        // Weight by cosine term since Epic claims it generally improves quality.
        for(uint i = 0; i < NumSamples; ++i)
        {
            vec2 u = SampleHammersley(i);
            vec3 Lh = TangentToWorld(sampleGGX(u.x, u.y, roughness), N, S, T);

            // Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
            vec3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

            float cosLi = dot(N, Li);
            if(cosLi > 0.0)
            {
                // Use Mipmap Filtered Importance Sampling to improve convergence.
                // See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

                float cosLh = max(dot(N, Lh), 0.0);

                // GGX normal distribution function (D term) probability density function.
                // Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
                float pdf = ndfGGX(cosLh, roughness) * 0.25;

                // Solid angle associated with this sample.
                float ws = 1.0 / (NumSamples * pdf);

                // Mip level to sample from.
                float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

                color += textureLod(s_environment, Li, mipLevel).rgb * cosLi;
                //color += Li;
                weight += cosLi;
            }
        }
        color /= weight;
    }

    imageStore(s_prefiltered, ivec3(gl_GlobalInvocationID), vec4(color, 1.0));
}
