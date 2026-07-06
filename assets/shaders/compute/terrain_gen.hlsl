RWStructuredBuffer<float> height_map : register(u0, space0);

[[vk::image_format("rgba8")]]
RWTexture2D<unorm float4> height_map_image : register(u1, space0);

struct Terrain_Generation_Settings
{
    uint terrain_size;
    float frequency;
    float amplitude;
};

struct Push_Constants
{
    vk::BufferPointer<Terrain_Generation_Settings> terrain_gen_settings;
};

[[vk::push_constant]]
Push_Constants push_constants;

float noise(float2 coords)
{
    Terrain_Generation_Settings terrain_generation_settings = push_constants.terrain_gen_settings.Get();
    
    float value = coords.x + coords.y;
    float scaled = value / (terrain_generation_settings.frequency * terrain_generation_settings.terrain_size * 2);
    return scaled * 2.0 - 1.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    Terrain_Generation_Settings terrain_generation_settings = push_constants.terrain_gen_settings.Get();
    
    uint terrain_size = terrain_generation_settings.terrain_size;
    float frequency = terrain_generation_settings.frequency;
    float amplitude = terrain_generation_settings.amplitude;
    
    float noise_value = noise(float2(dispatch_thread_id.xy) * frequency);
    height_map[dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = noise_value * amplitude;
    
    float scaled_noise_value = clamp((noise_value + 1.0) / 2.0, 0.0, 1.0);
    float4 final_color = float4(max(0.5 - noise_value, 0.0) * 2.0, 0, max(noise_value - 0.5, 0.0) * 2.0, 1);
    height_map_image[uint2(dispatch_thread_id.x, dispatch_thread_id.y)] = final_color.bgra; // swizzle to match actual bgra8 format
}
