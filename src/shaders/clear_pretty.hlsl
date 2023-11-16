#include "inputs/pos_uv.hlsl"

[shader("pixel")]
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    target_0 = float4(psIn.Uv.x * 2.0, psIn.Uv.y * 2.0, 0.0, 1.0);
}
