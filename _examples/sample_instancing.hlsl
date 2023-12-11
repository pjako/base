[RenderProgram]
name = SampleInstancingProgram
code = InstancingCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = InstancingCode

struct VSInput {
    [[vk::location(0)]] float3 pos : pos;
    [[vk::location(1)]] float4 color : color0;
    [[vk::location(2)]] float3 instancePos : instancePos;
};

struct PSInput {
    float4 pos          : SV_POSITION;
    float4 col          : COLOR0;
};

[[rx::dynResGroup0()]] float turnRate();

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    PSInput output;
    output.pos = float4(input.pos + input.instancePos, 1);
    output.col = input.color;

    return output;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.col;
}
