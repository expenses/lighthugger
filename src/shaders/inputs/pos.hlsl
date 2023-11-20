
struct V2P
{
    [[vk::location(0)]] float4 Pos : SV_Position;
    [[vk::location(1)]] float3 world_pos : COLOR0;
    [[vk::location(2)]] float3 normal: COLOR1;
    [[vk::location(3)]] float2 uv: COLOR2;
    [[vk::location(4)]] uint material_index: COLOR3;
    [[vk::location(5)]] uint64_t material_info_address: COLOR4;
};
