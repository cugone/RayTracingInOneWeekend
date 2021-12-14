struct ps_input {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
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

float4 main(ps_input input) : SV_TARGET {
	return input.color;
}
