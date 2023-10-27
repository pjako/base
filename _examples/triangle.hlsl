[RenderProgram]
name = TriangleProgram
code = TriangleCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = TriangleCode
struct VSInput {
    [[vk::location(0)]] float2 dstTopLeft :   vtl;
    [[vk::location(1)]] float2 dstBottomRight : vbr;
    //[[vk::location(2)]] float2 srcTopLeft : utl;
    //[[vk::location(3)]] float2 srcBottomRight : ubr;
    [[vk::location(2)]] float4 color : color;
    [[vk::location(3)]] float2 uv : coords;
};

struct PSInput {
	float4 pos          : SV_POSITION;
    float4 col          : COLOR0;
    float2 uv           : TEXCOORD0;
    float2 rectCenter   : foo;
    float2 rectHalfSize : bar;
    float2 rectPos      : baz;
};

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
	PSInput output;
	return output;
}

float roundedRectSDF(float2 sample_pos, float2 rect_center, float2 rect_half_size, float r) {
  float2 d2 = (abs(rect_center - sample_pos) - rect_half_size + float2(r, r));
  return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}

float4 psMain(PSInput input) : SV_TARGET {

    return color;
}