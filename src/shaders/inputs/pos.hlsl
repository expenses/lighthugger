
struct V2P
{
    [[vk::location(0)]] float4 Pos : SV_Position;
    [[vk::location(1)]] float3 world_pos : COLOR0;
    [[vk::location(2)]] float3 normal: COLOR1;
};
