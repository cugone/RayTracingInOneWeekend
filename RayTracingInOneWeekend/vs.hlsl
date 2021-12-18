struct ps_input {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct vs_input {
    float3 pos : POSITION;
    float3 color : COLOR;
    float2 uv : UV;
};

cbuffer world_constants : register(b0) {
    float gScreenWidth;
    float gScreenHeight;
};

cbuffer frame_constants : register(b1) {
    float4x4 gMVPMatrix;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float gGameTime;
    float gGameFrameTime;
    float gSamplesPerPixel;
    float gMaxDepth;
};

cbuffer object_constants : register(b2) {
    float4x4 gModelMatrix;
};

ps_input main(vs_input input) {
    ps_input output;

    float4 pos = float4(input.pos, 1.0f);
    float4 color = float4(input.color, 1.0f);

    output.position = mul(pos, gMVPMatrix);

    output.color = color;
    output.uv = input.uv;

    return output;
}
