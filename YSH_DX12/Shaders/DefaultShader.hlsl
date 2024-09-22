Texture2D texDiffuse : register(t0);   
SamplerState samplerDiffuse	: register(s0);
cbuffer CONSTANT_BUFFER_DEFAULT : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Proj;
};

struct VSInput
{
    float4 Pos : POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    
    PSInput result = (PSInput)0;


    matrix matViewProj = mul(g_View, g_Proj);
    matrix matWorldViewProj = mul(g_World, matViewProj);

    
    result.position = mul(input.Pos, matWorldViewProj);
    result.TexCoord = input.TexCoord;
    result.color = input.color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4	texColor = texDiffuse.Sample(samplerDiffuse, input.TexCoord);
    return texColor* input.color;
}
