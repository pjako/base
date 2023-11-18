[RenderProgram]
name = SampleTextureProgram
code = TexSampleCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = TexSampleCode
struct VSInput {
    [[vk::location(0)]] float3 pos : pos;
    [[vk::location(1)]] float2 coords0 : coords0;
};

struct PSInput {
    float4 pos          : SV_POSITION;
    float2 uv           : TEXCOORD0;
};

[[rx::resGroup1()]] Texture2D myTexture();
[[rx::resGroup1()]] SamplerState mySampler();
[[rx::dynResGroup0()]] float2 transformOffset();

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    PSInput output;
    output.pos = float4(input.pos.xyz + float3(transformOffset(), 0), 1);
    output.uv  = input.coords0;

    return output;
}

float4 psMain(PSInput input) : SV_TARGET {
    return myTexture().Sample(mySampler(), input.uv);
}
