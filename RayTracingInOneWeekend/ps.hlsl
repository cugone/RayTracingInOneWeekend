struct ps_input {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

float4 main(ps_input input) : SV_TARGET {
	return input.color;
}
