RWBuffer<float> height_map : register(u0);

[[vk::image_format("rgba8")]]
RWTexture2D<unorm float4> height_map_image : register(u0);

cbuffer TerrainGenerationSettings : register(b0)
{
    uint terrain_size;
    float frequency;
    float amplitude;
};

float noise(float2 coords)
{
    return 0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    float noise_value = noise(float2(dispatch_thread_id.xy) * frequency) * amplitude;
    height_map[dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = noise_value;
    
    float scaled_noise_value = clamp((noise_value + 1.0) / 2.0, 0.0, 1.0);
    float4 final_color = float4(max(0.5 - noise_value, 0.0) * 2.0, 0, max(noise_value - 0.5, 0.0) * 2.0, 1);
    height_map_image[uint2(dispatch_thread_id.x, dispatch_thread_id.y)] = final_color.bgra; // swizzle to match actual bgra8 format
}
