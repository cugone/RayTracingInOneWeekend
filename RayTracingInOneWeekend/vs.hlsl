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

ps_input main(vs_input input) {
    ps_input output;

    float4 pos = float4(input.pos, 1.0f);
    float4 color = float4(input.color, 1.0f);

    output.position = pos;
    output.color = color;
    output.uv = input.uv;

    return output;
}
