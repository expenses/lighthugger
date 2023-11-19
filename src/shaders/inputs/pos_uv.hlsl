
struct V2P
{
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] float2 uv : COLOR0;
};
