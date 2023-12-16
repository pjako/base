[RenderProgram]
name = CubeProgram
code = CubeCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = CubeCode

struct VSInput {
    [[vk::location(0)]] float3 pos : pos;
    [[vk::location(1)]] float4 color : color0;
};

struct PSInput {
    float4 pos          : SV_POSITION;
    float4 col          : COLOR0;
};

[[rx::dynResGroup0()]] float4x4 mvp();

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    PSInput output;
    output.pos = mul(float4(input.pos, 1.0f), mvp());
    output.col = input.color;

    return output;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.col;
}
