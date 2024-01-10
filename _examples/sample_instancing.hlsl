[RenderProgram]
name = SampleInstancingProgram
code = InstancingCode
vsEntry = vsMain
psEntry = psMain
[ResGroup]
name = TransformData
genName = TransformDataResGroup
mvp = float4x4
[Code]
name = InstancingCode
dynResGroup0 = TransformData

struct VSInput {
    [[vk::location(0)]] float3 pos : pos;
    [[vk::location(1)]] float4 color : color0;
    [[vk::location(2)]] float3 instancePos : instancePos;
};

struct PSInput {
    float4 pos          : SV_POSITION;
    float4 col          : COLOR0;
};

[[rx::dynResGroup0()]] float4x4 mvp();

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    PSInput output;
    float4 pos = float4(input.pos + input.instancePos, 1);
    output.pos = mul(pos, mvp());
    output.col = input.color;

    return output;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.col;
}
