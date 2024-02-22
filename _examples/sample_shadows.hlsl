[RenderProgram]
name = ShadowProgram
code = ShadowCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = ShadowCode
float4 encodeDepth(float v) {
    float4 enc = float4(1.0, 255.0, 65025.0, 16581375.0) * v;
    enc = frac(enc);
    enc -= enc.yzww * float4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);
    return enc;
}

float decodeDepth(float4 rgba) {
    return dot(rgba, float4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

float sampleShadow(Texture2D tex, SamplerState smp, float2 uv, float compare) {
    float depth = decodeDepth(tex.Sample(smp, uv));
    return step(compare, depth);
}

// PCF for soft edges
float sampleShadowPcf(Texture2D tex, SamplerState smp, float3 uv_depth, float2 sm_size) {
    float result = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y =- 2; y <= 2; y++) {
            float2 offset = float2(x, y) / sm_size;
            result += sampleShadow(tex, smp, uv_depth.xy + offset, uv_depth.z);
        }
    }
    return result / 25.0;
}

[[rx::resGroup1()]] Texture2D myTexture();
[[rx::resGroup1()]] SamplerState mySampler();

[[rx::dynResGroup0()]] float4x4 mvp();
[[rx::dynResGroup0()]] float4x4 model();
[[rx::dynResGroup0()]] float4x4 lightMvp();
[[rx::dynResGroup0()]] float3 diffColor();
[[rx::dynResGroup0()]] float3 lightDir();
[[rx::dynResGroup0()]] float3 eyePos();

struct VSInput {
    [[vk::location(0)]] float3 pos : pos;
    [[vk::location(1)]] float3 norm : normal;
};

struct PSInput {
    float4 pos          : SV_POSITION;
    float3 col          : COLOR0;
    float4 normal       : NORMAL0;
    float4 lightProjPos : LIGHTPOS0;
    float4 worldPos     : WORLDPOS;
    float4 worldNorm    : WORLDNORM;
};

PSInput vsMain(VSInput input, uint vertexIndex : SV_VertexID) {
    PSInput output;
    output.pos = mul(float4(input.pos, 1.0f), mvp());
    output.lightProjPos = mul(float4(input.pos, 1.0f), lightMvp());


    output.worldPos = mul(float4(input.pos, 1.0f), model());
    output.worldNorm = normalize(float4(mul(float4(input.norm, 1.0f), model()).xyz, 0.0f));
    output.col = diffColor();

    return output;
}
/*
float4 gamma(float4 c) {
    float p = 1.0 / 2.2;
    return float4(pow(c.xyz, float3(p)), c.w);
}
*/

float4 psMain(PSInput input) : SV_TARGET {
    return float4(input.col, 1);
}