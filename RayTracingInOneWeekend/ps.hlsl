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
    float gVFovRadians;
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

ray get_ray(float s, float t) {
    const float theta = gVFovRadians;
    const float h = tan(theta * 0.5f);
    const float viewport_height = 2.0f * h;
    const float viewport_width = gAspectRatio * viewport_height;

    const float3 u = float3(gViewMatrix[0].x, gViewMatrix[0].y, gViewMatrix[0].z);
    const float3 v = float3(gViewMatrix[1].x, gViewMatrix[1].y, gViewMatrix[1].z);
    const float3 w = float3(gViewMatrix[2].x, gViewMatrix[2].y, gViewMatrix[2].z);

    const float3 horizontal = gFocusDistance * viewport_width * u;
    const float3 vertical = gFocusDistance * viewport_height * v;

    const float4 random_disk = gLensRadius * ((tRandomDiskResult.Sample(sSampler, float2(s, t)) * 2.0f) - 1.0f);
    const float3 offset = u * random_disk.x + v * random_disk.y;
    const float3 lower_left_corner = gEyePosition - horizontal * 0.5f - vertical * 0.5f - gFocusDistance * w;

    ray r;
    r.position = gEyePosition + offset;
    r.direction = lower_left_corner + s * horizontal + t * vertical - gEyePosition - offset;

    return r;
}

uniform float pi = 3.141592653589793f;

float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}


float4 main(ps_input input) : SV_TARGET{
    const float pixel_x = 1.0f / gScreenWidth;
    const float pixel_y = 1.0f / gScreenHeight;

    [unroll]
    for (int y = gScreenHeight - pixel_x; y >= 0.0f; y -= pixel_y) {
        [unroll]
        for (int x = 0; x < gScreenWidth; x += pixel_x) {
            float4 pixel_color = float4(0.0f, 0.0f, 0.0f, 1.0f);
            [unroll]
            for (int current_sample = 0; current_sample < gSamplesPerPixel / 2; ++current_sample) {
                const float u = (x + tRandomUV.Sample(sSampler, current_sample / gSamplesPerPixel).r) / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.Sample(sSampler, current_sample / gSamplesPerPixel).g) / (gScreenHeight - pixel_y);
                const ray r = get_ray(u, v);
                pixel_color += ray_color(r, gMaxDepth);
            }
            [unroll]
            for (int current_sample = gSamplesPerPixel / 2; current_sample < gSamplesPerPixel; ++current_sample) {
                const float u = (x + tRandomUV.Sample(sSampler, current_sample / gSamplesPerPixel).b) / (gScreenWidth - pixel_x);
                const float v = (y + tRandomUV.Sample(sSampler, current_sample / gSamplesPerPixel).a) / (gScreenHeight - pixel_y);
                const ray r = get_ray(u, v);
                pixel_color += float4(ray_color(r, gMaxDepth), 1.0f);
            }
        }
    }
    return pixel_color;
}
