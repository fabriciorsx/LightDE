// simple_shaders.hlsl
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

cbuffer cbPerFrame : register(b1)
{
    float3 gCameraPos;
};

cbuffer cbLight : register(b2)
{
    float3 gLightPos;
};

cbuffer cbLightColor : register(b3)
{
    float3 gLightColor;
};

cbuffer cbWorld : register(b4)
{
    float4x4 gWorld;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // Transladar para posição da luz
    float3 worldPos = vin.PosL + gLightPos;
    vout.PosH = mul(float4(worldPos, 1.0f), gWorldViewProj);
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(pin.Color, 0.8f); // Semi-transparente
}