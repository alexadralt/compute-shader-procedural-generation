RWStructuredBuffer<float>  height_map        : register(u0, space0);
RWStructuredBuffer<float2> norm_gradient_map : register(u1, space0);

float2 bilinear_gradient_map(float2 current_pos, uint terrain_size)
{
    float2 grid_coords = floor(current_pos);
    uint2 uv = uint2(grid_coords) & (terrain_size - 1);
    
    float2 hx0y0 = norm_gradient_map[  uv.x                            * terrain_size +   uv.y                           ];
    float2 hx1y0 = norm_gradient_map[((uv.x + 1) & (terrain_size - 1)) * terrain_size +   uv.y                           ];
    float2 hx0y1 = norm_gradient_map[  uv.x                            * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    float2 hx1y1 = norm_gradient_map[((uv.x + 1) & (terrain_size - 1)) * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    
    float t = current_pos.x - grid_coords.x;
    float s = current_pos.y - grid_coords.y;
    
    return hx0y0 * (1 - t) * (1 - s) + hx1y0 * t * (1 - s) + hx0y1 * (1 - t) * s + hx1y1 * t * s;
}

float bilinear_height_map(float2 current_pos, uint terrain_size)
{
    float2 grid_coords = floor(current_pos);
    uint2 uv = uint2(grid_coords) & (terrain_size - 1);
    
    float hx0y0 = height_map[  uv.x                            * terrain_size +   uv.y                           ];
    float hx1y0 = height_map[((uv.x + 1) & (terrain_size - 1)) * terrain_size +   uv.y                           ];
    float hx0y1 = height_map[  uv.x                            * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    float hx1y1 = height_map[((uv.x + 1) & (terrain_size - 1)) * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    
    float t = current_pos.x - grid_coords.x;
    float s = current_pos.y - grid_coords.y;
    
    return hx0y0 * (1 - t) * (1 - s) + hx1y0 * t * (1 - s) + hx0y1 * (1 - t) * s + hx1y1 * t * s;
}

inline float smax(float x, float y, float lambda)
{
    return (x + y + sqrt((x - y) * (x - y) + lambda)) / 2;
}


inline float terrain_sdf(float3 current_pos, float3 norm_ray_direction, uint terrain_size)
{
    float value = bilinear_height_map(current_pos.xz, terrain_size) - current_pos.y;
    return value * 0.5f; // factor to avoid holes at steep slopes
}

inline float sphere_sdf(float3 current_pos)
{
    return length(current_pos - float3(512, -100, 490)) - 25.f;
}

#define Max_Steps 256
#define Eps 0.001

struct Raymarch_Result
{
    float3 position;
    bool   hit;
};

Raymarch_Result raymarch(float3 ray_start_pos, float3 norm_ray_direction, uint terrain_size)
{
    Raymarch_Result result;
    result.hit = false;
    
    float3 current_pos = ray_start_pos;
    for (int step = 0; step < Max_Steps; ++step)
    {
        float distance = terrain_sdf(current_pos, norm_ray_direction, terrain_size);
        if (abs(distance) < Eps)
        {
            float2 grid_pos = floor(current_pos.xz);
            if (grid_pos.x < 0 || grid_pos.y < 0 || grid_pos.x >= float(terrain_size * 5) - Eps || grid_pos.y >= float(terrain_size * 5) - Eps)
            {
                return result;
            }
            
            result.position = current_pos;
            result.hit = true;
            return result;
        }
        
        current_pos += distance * norm_ray_direction;
    }
    
    return result;
}

float4 get_color_for_ray(float3 ray_start_pos, float3 norm_ray_direction, uint terrain_size)
{
    const float3 sun_direction = normalize(float3(1, -0.3f, 1));
    
    Raymarch_Result result = raymarch(ray_start_pos, norm_ray_direction, terrain_size);
    if (result.hit)
    {
        // diffuse lightning
        float2 grad = normalize(bilinear_gradient_map(result.position.xz, terrain_size));
        float3 normal = normalize(float3(grad.x, -1, grad.y));
        float diffuse = 1 - max(dot(sun_direction, normal), 0);
        const float3 ambient_color = float3(0.1, 0.1, 0.1);
        float3 color = lerp(float3(0.6, 0.6, 0.2), ambient_color, diffuse);
        
        // shadows
        Raymarch_Result shadow_result = raymarch(result.position + sun_direction * 10, sun_direction, terrain_size);
        if (shadow_result.hit)
        {
            color = ambient_color;
        }
        
        const float base_height = 50;
        float3 base_color = 0.5 * lerp(float3(1, 0, 0), float3(0, 0, 1), smoothstep(-base_height, base_height, clamp(-result.position.y, -base_height, base_height)));
        color += base_color;
        
        return float4(color, 1);
    }
    
    // sky
    float t = norm_ray_direction.y * 0.5f + 0.5f;
    float3 sky_color = lerp(float3(0.6f, 0.6f, 0.9f), float3(0.1f, 0.1f, 0.2f), t);
    
    const float sun_size = 0.999f;
    float s = max(dot(norm_ray_direction, sun_direction), sun_size);
    const float3 sun_color = float3(1.f, 1.f, 0.1f);
    
    float3 final_color = lerp(sky_color, sun_color, smoothstep(sun_size, 1.f, s));
    return float4(final_color, 1);
}

[[vk::image_format("rgba8")]]
RWTexture2D<unorm float4> out_image : register(u2, space0);

struct Camera_Info
{
    float3 positon;
    float3x3 world_from_camera;
};

struct Raymarch_Info
{
    Camera_Info camera_info;
    uint2 out_image_size;
    uint terrain_size;
};

struct Push_Constants
{
    vk::BufferPointer<Raymarch_Info> raymarch_info;
};

[[vk::push_constant]]
Push_Constants push_constants;

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    Raymarch_Info raymarch_info = push_constants.raymarch_info.Get();
    Camera_Info camera_info = raymarch_info.camera_info;
    float2 pixel_coords = float2(dispatch_thread_id.xy);
    
    // normalize
    pixel_coords.x /= float(raymarch_info.out_image_size.x);
    pixel_coords.y /= float(raymarch_info.out_image_size.y);
    
    pixel_coords -= 0.5;

    pixel_coords.x *= float(raymarch_info.out_image_size.x) / float(raymarch_info.out_image_size.y); // adjust for aspect ratio
    
    float3 ray_direction = float3(pixel_coords.x, pixel_coords.y, 1);
    float3 world_ray_direction = normalize(mul(ray_direction, camera_info.world_from_camera));
    
    float4 color = get_color_for_ray(camera_info.positon, world_ray_direction, raymarch_info.terrain_size).bgra; // swizzle to match actual bgra layout
    out_image[dispatch_thread_id.xy] = float4(pow(color.rgb, 0.4545), 1.f); // gamma correction
}
