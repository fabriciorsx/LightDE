cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

cbuffer cbCameraData : register(b1)
{
    float3 gCameraPos;
    float padding1;
};

cbuffer cbLightData : register(b2)
{
    float3 gLightPos;
    float padding2;
};

cbuffer cbLightColor : register(b3)
{
    float3 gLightColor;
    float padding3;
};

cbuffer cbWorldMatrix : register(b4)
{
    float4x4 gWorld;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 Albedo : ALBEDO;
    float Metallic : METALLIC;
    float Roughness : ROUGHNESS;
    float AO : AO;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 Albedo : ALBEDO;
    float Metallic : METALLIC;
    float Roughness : ROUGHNESS;
    float AO : AO;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.Albedo = vin.Albedo;
    vout.Metallic = vin.Metallic;
    vout.Roughness = vin.Roughness;
    vout.AO = vin.AO;
    return vout;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 albedo = pin.Albedo;
    float metallic = pin.Metallic;
    float roughness = pin.Roughness;
    float ao = pin.AO;

    float3 N = normalize(pin.NormalW);
    float3 V = normalize(gCameraPos - pin.PosW);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 L = normalize(gLightPos - pin.PosW);
    float3 H = normalize(V + L);
    float distance = length(gLightPos - pin.PosW);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = gLightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedo / 3.14159265 + specular) * radiance * NdotL;

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float3 color = ambient + Lo;
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1.0);
}
