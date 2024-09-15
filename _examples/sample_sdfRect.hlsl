[RenderProgram]
name = UIProgram
code = UIShaderCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = UIShaderCode
dynResGroup0 = UIDynResDetails
struct VSInput {
    [[vk::location(0)]] float2 dstTopLeft :   vtl;
    [[vk::location(1)]] float2 dstBottomRight : vbr;
    //[[vk::location(2)]] float4 color : color;
    //[[vk::location(3)]] float2 uvTopLeft : uvTopLeft;
    //[[vk::location(4)]] float2 uvBottomRight : uvBottomRight;
};
// float outline_thickness = 0.1;
// float4 outline_color = float4(color.xyz, 1);
// float4 shadow_color = float4(1, 1, 0, 1);
// float2 shadowOffset = float2(0.001, 0.001);

struct PSInput {
	float4 pos              : SV_POSITION;
    float2 rectCenter       : rectCenter;
    float2 rectHalfSize     : rectHalfSize;
    float2 rectPos          : rectPos;
    float3 col              : COLOR0;
};

[[rx::resGroup1()]] Texture2D atlasTexture();
[[rx::resGroup1()]] SamplerState atlasSampler();

[[rx::dynResGroup0()]] float2 screenSize();

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    // static vertex array that we can index into
    // with our vertex ID
    static float2 vertices[] =
    {
        {-1, +1},
        {-1, -1},
        {+1, -1},
        {+1, +1},
    };

    // -100 -100
    // 100 100
    float2 screenRes = screenSize();
    float2 screenResHalf = screenRes / 2;

    // "dst" => "destination" (on screen)
    float2 vertex = vertices[vertexIndex % 4];

    float2 tl =   input.dstTopLeft; //max(input.dstTopLeft, screenResHalf + float2(-120, 120)); // 67.5, 454
    float2 br =  input.dstBottomRight; //max(input.dstBottomRight, screenResHalf + float2(120, -120)); // 307.5,  214

    // vertex pos
    float2 dst_half_size = (br - tl) / 2; // ((-100, -100) - (100, 100)) /  2 = (-100, -100) // 240, -240
    float2 dst_center = (tl + dst_half_size); // ((100, 100) + (-100, -100)) / 2 = (0, 0)
    float2 dst_pos = (vertex * dst_half_size + dst_center); // ((-1, -1) * (-100, -100))

    // uv pos
    //float2 uvDstHalfSize = (input.uvBottomRight - input.uvTopLeft) / 2; // ((-100, -100) - (100, 100)) /  2 = (-100, -100)
    //float2 uvDstCenter = (input.uvBottomRight + input.uvTopLeft) / 2; // ((100, 100) + (-100, -100)) / 2 = (0, 0)
    //float2 uvDstPos = (vertex * uvDstHalfSize + uvDstCenter); // ((-1, -1) * (-100, -100))


	PSInput output;
	//output.col = float4(input.color.xyz, min(input.color.w, 1.0));
    output.pos = float4(2 * dst_pos.x / screenRes.x - 1, 2 * dst_pos.y / screenRes.y - 1, 0, 1);
    // output.pos = (output.pos * 0.0f) + float4(vertex, 0, 1);
    output.rectCenter = dst_center;
    output.rectHalfSize = dst_half_size;
    output.rectPos = dst_pos;
    output.col = float3(0, 1, 1);
	//output.uv = uvDstPos;
	return output;
}

float roundedRectSDF(float2 sample_pos, float2 rect_center, float2 rect_half_size, float r) {
  float2 d2 = (abs(rect_center - sample_pos) - rect_half_size + float2(r, r));
  return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float map(float min, float max, float v) {
    return (v - min) / (max - min);
}

float4 psMain(PSInput input) : SV_TARGET {
    // float2 rect = iResolution.xy / 2.0f;
    float2 rectHalf = input.rectHalfSize;
    float2 rectCenter = input.rectCenter;
    float2 currPos = input.rectPos;// * iResolution.xy;
    
    float edgeSoftness  = 1.2f;
    float2 softness = float2(edgeSoftness, edgeSoftness);
    float2 softness_padding = float2(max(0.0f, softness.x*2.0f-1.0f),max(0.0f, softness.x*2.0f-1.0f));
    

    // Rect
    float dist = roundedRectSDF(currPos, rectCenter, rectHalf - softness_padding, 20.0f);
    float sdfFactor = 1.f - smoothstep(0.0f, 2.0f*softness.x, dist);

    //float3 combineEffects = lerp(normalize(col), backgroundColor, 1.0f - sdfFactor);

    //return float4(combineEffects,1);
    return float4(input.col, max(sdfFactor, 0.1f));
}