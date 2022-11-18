Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

#include "ChromAbb.hlsli"

float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target0
{
    float2 uv = texCoord.xy;

    float amount = 0.05f;
        
    amount = (1.0 + sin(time * 6.0)) * 0.5;
    amount *= 1.0 + sin(time * 16.0) * 0.5;
    amount *= 1.0 + sin(time * 19.0) * 0.5;
    amount *= 1.0 + sin(time * 27.0) * 0.5;
    amount = pow(amount, 3.0);
    
    amount *= 0.05;
    
    float4 c = Texture.Sample(TextureSampler, texCoord);
    float4 col;
        
    col.r = Texture.Sample(TextureSampler, float2(uv.x + amount, uv.y-amount)).r;
    col.g = Texture.Sample(TextureSampler, uv).g;
    col.b = Texture.Sample(TextureSampler, float2(uv.x - amount, uv.y)).b;
    col.a = 1;
 
    col *= (1.0f - amount * 0.5);
    
    
    return col;
}

//Texture2D<float4> Texture : register(t0);
//sampler TextureSampler : register(s0);


//#include "ChromAbb.hlsli"


//float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target0
//{
   
//    float4 c = Texture.Sample(TextureSampler, texCoord);

//    c.r = time;
//    c.g = 0;
//    c.b = 0;
//    c.a = 1;
    
//    return c; //float4((0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b), (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b), (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b), 1);
//}


