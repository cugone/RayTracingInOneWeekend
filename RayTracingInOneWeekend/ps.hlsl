struct ps_input {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float4 world : POSITION;
};

cbuffer world_constants : register(b0) {
    float gScreenWidth;
    float gScreenHeight;
    float2 gWorldPadding;
};

cbuffer frame_constants : register(b1) {
    float4x4 gViewProjectionMatrix;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float gGameTime;
    float gGameFrameTime;
    float gSamplesPerPixel;
    float gMaxDepth;
    float3 gEyePosition;
    float3 gLookAt;
    float gVFovDegrees;
    float gAspectRatio;
    float gAperture;
    float gLensRadius;
    float gFocusDistance;
    float gFramePadding;
};

cbuffer object_constants : register(b2) {
    float4x4 gModelMatrix;
    float4 radius_padding3;
};

SamplerState sSampler : register(s0);

Texture1D<float4> tRandomUV : register(t0);
Texture2D<float4> tRandomDiskResult : register(t1);

struct ray {
    float3 position;
    float3 direction;
};

struct camera {
    float3 origin;
    float3 lower_left_corner;
    float3 horizontal;
    float3 vertical;
    float3 u;
    float3 v;
    float3 w;
    float lens_radius;
};

ray get_ray(camera c, float s, float t) {
    const float4 random_disk = c.lens_radius * ((tRandomDiskResult.Sample(sSampler, float2(s, t)) * 2.0f) - 1.0f);
    const float3 offset = c.u * random_disk.x + c.v * random_disk.y;
    ray r;
    r.position = c.origin + offset;
    r.direction = c.lower_left_corner + s * c.horizontal + t * c.vertical - c.origin - offset;
    return r;
}

uniform float pi = 3.141592653589793f;

float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}


float4 main(ps_input input) : SV_TARGET{
    const float pixel_x = 1.0f / gScreenWidth;
    const float pixel_y = 1.0f / gScreenHeight;

    camera c = make_camera(gEyePosition, gLookAt, float4(0.0f, 1.0f, 0.0, 0.0f), gVFovDegrees, gAspectRatio, gAperture, gFocusDistance);

    [unroll]
    for (int y = gScreenHeight - pixel_x; y >= 0.0f; y -= pixel_y) {
        [unroll]
        for (int x = 0; x < gScreenWidth; x += pixel_x) {
            float4 pixel_color = float4(0.0f, 0.0f, 0.0f, 1.0f);
            [unroll]
            for (int current_sample = 0; current_sample < gSamplesPerPixel / 2; ++current_sample) {
                const float u = (x + tRandomUV.Sample(sSampler, current_sample / gSamplersPerPixel).r / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.Sample(sSampler, current_sample / gSamplersPerPixel).g) / (gScreenHeight - pixel_y);
                const float r = get_ray(c, u, v);
                pixel_color += ray_color(r, gMaxDepth);
            }
            [unroll]
            for (int current_sample = gSamplesPerPixel / 2; current_sample < gSamplesPerPixel; ++current_sample) {
                const float u = (x + tRandomUV.Sample(sSampler, current_sample / gSamplersPerPixel).b / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.Sample(sSampler, current_sample / gSamplersPerPixel).a) / (gScreenHeight - pixel_y);
                const float r = get_ray(c, u, v);
                pixel_color += ray_color(r, gMaxDepth);
            }
        }
    }
    return pixel_color;
}
