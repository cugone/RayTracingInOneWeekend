struct ps_input {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct vs_input {
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

ps_input main(vs_input input) {
	ps_input output;

	float4 pos = float4(input.pos, 1.0f);

	output.position = pos;
	output.color = input.color;
	output.uv = input.uv;

	return output;
}
