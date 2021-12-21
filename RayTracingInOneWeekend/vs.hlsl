struct ps_input {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float4 world : POSITION;
};

struct vs_input {
    float3 pos : POSITION;
    float3 color : COLOR;
    float2 uv : UV;
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
    float gVFovDegrees;
    float gAspectRatio;
    float gLensRadius;
};

cbuffer object_constants : register(b2) {
    float4x4 gModelMatrix;
};

ps_input main(vs_input input) {
    ps_input output;

    float4 local = float4(input.pos, 1.0f);
    float4 color = float4(input.color, 1.0f);
    float4 world = mul(local, gModelMatrix);
    float4 clip = mul(world, gViewProjectionMatrix);

    output.position = clip;
    output.color = color;
    output.uv = input.uv;
    output.world = world;

    return output;
}
