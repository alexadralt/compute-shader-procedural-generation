StructuredBuffer<float>  height_map[MAX_DESCRIPTOR_ARRAY_SIZE]        : register(t0, space0);
StructuredBuffer<float2> norm_gradient_map[MAX_DESCRIPTOR_ARRAY_SIZE] : register(t1, space0);

[[vk::image_format("rgba8")]]
RWTexture2D<unorm float4> out_image : register(u2, space0);

struct Camera_Info
{
    float3 positon;
    float3x3 world_from_camera;
};

enum Render_Type
{
    Render_Type_Regular,
    Render_Type_Normals,
};

struct Raymarch_Info
{
    Camera_Info camera_info;
    uint2 out_image_size;
    uint terrain_size;
    uint chunk_count_x;
    Render_Type render_type;
};

struct Push_Constants
{
    vk::BufferPointer<Raymarch_Info> raymarch_info;
};

[[vk::push_constant]]
Push_Constants push_constants;

inline uint get_map_index(uint2 st)
{
    Raymarch_Info raymarch_info = push_constants.raymarch_info.Get();
    uint terrain_size = raymarch_info.terrain_size;
    uint render_distance = raymarch_info.chunk_count_x;
    
    uint chunk_x = (st.x / terrain_size) % render_distance;
    uint chunk_y = (st.y / terrain_size) % render_distance;
    return chunk_x * render_distance + chunk_y;
}

template<typename T>
T bilinear_map(StructuredBuffer<T> map[MAX_DESCRIPTOR_ARRAY_SIZE], float2 current_pos)
{
    Raymarch_Info raymarch_info = push_constants.raymarch_info.Get();
    uint terrain_size = raymarch_info.terrain_size;
    
    float2 grid_coords = floor(current_pos);
    uint2 st = uint2(grid_coords);
    uint2 uv = st & (terrain_size - 1);
    T hx0y0 = map[get_map_index(st              )][  uv.x                            * terrain_size +   uv.y                           ];
    T hx1y0 = map[get_map_index(st + uint2(1, 0))][((uv.x + 1) & (terrain_size - 1)) * terrain_size +   uv.y                           ];
    T hx0y1 = map[get_map_index(st + uint2(0, 1))][  uv.x                            * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    T hx1y1 = map[get_map_index(st + uint2(1, 1))][((uv.x + 1) & (terrain_size - 1)) * terrain_size + ((uv.y + 1) & (terrain_size - 1))];
    
    float t = current_pos.x - grid_coords.x;
    float s = current_pos.y - grid_coords.y;
    
    return hx0y0 * (1 - t) * (1 - s) + hx1y0 * t * (1 - s) + hx0y1 * (1 - t) * s + hx1y1 * t * s;
}

inline float2 bilinear_gradient_map(float2 current_pos)
{
    return bilinear_map(norm_gradient_map, current_pos);
}

inline float bilinear_height_map(float2 current_pos)
{
    return bilinear_map(height_map, current_pos);
}

inline float smax(float x, float y, float lambda)
{
    return (x + y + sqrt((x - y) * (x - y) + lambda)) / 2;
}

inline float terrain_sdf(float3 current_pos)
{
    float value = bilinear_height_map(current_pos.xz) - current_pos.y;
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

Raymarch_Result raymarch(float3 ray_start_pos, float3 norm_ray_direction)
{
    Raymarch_Info raymarch_info = push_constants.raymarch_info.Get();
    uint terrain_size = raymarch_info.terrain_size;
    uint chunk_count_x = raymarch_info.chunk_count_x;
    
    Raymarch_Result result;
    result.hit = false;
    
    float3 current_pos = ray_start_pos;
    float3 prev_pos = ray_start_pos;
    for (int step = 0; step < Max_Steps; ++step)
    {
        float distance = terrain_sdf(current_pos);
        
        if (abs(distance) < Eps)
        {
            result.position = current_pos;
            result.hit = true;
            return result;
        }
        
        if (distance < Eps)
        {
            // do a binary search to find intersection point
            //
            // since distance ended up being negative we know for sure that intersection exists
            
            float3 left = current_pos;
            float3 right = prev_pos;
            for (int step = 0; step <= Max_Steps; ++step)
            {
                float3 middle = (left + right) * 0.5;
                float distance = terrain_sdf(middle);
                
                if (abs(distance) < Eps || step == Max_Steps)
                {
                    result.position = middle;
                    result.hit = true;
                    return result;
                }
                
                if (distance < -Eps)
                {
                    left = middle;
                }
                else
                {
                    right = middle;
                }
            }
        }
        
        prev_pos = current_pos;
        current_pos += distance * norm_ray_direction;
        
        // cull chunks outside the render distance
        float2 grid_pos = floor(current_pos.xz);
        if (grid_pos.x < 0 || grid_pos.y < 0 || grid_pos.x >= float(terrain_size * chunk_count_x) - Eps || grid_pos.y >= float(terrain_size * chunk_count_x) - Eps)
        {
            return result;
        }
    }
    
    return result;
}

static const float3 sun_direction = normalize(float3(1, -0.3f, 1));
static const float3 sun_color = float3(1.f, 1.f, 0.1f);
static const float3 ambient_color = float3(0.1, 0.1, 0.1) * Pi;
static const float albedo = 1.f;

inline float3 diffuse_specular(float3 norm_ray_direction, float3 base_color, float3 normal, float shininess)
{
    // diffuse lightning
    float3 diffuse_color = albedo * base_color * One_Over_Pi;
    float diffuse = max(dot(sun_direction, normal), 0);
    float3 diffuse_component = (ambient_color + sun_color * diffuse) * diffuse_color;
    
    // specular highlight
    float3 half_vector = normalize(sun_direction - norm_ray_direction);
    float specular = max(dot(half_vector, normal), 0.f) * step(0, dot(sun_direction, normal));
    const float3 specular_color = float3(1, 1, 1);
    float3 specular_component = sun_color * specular_color * pow(specular, shininess);
    
    return diffuse_component + specular_component;
}

inline float3 apply_shadow(float3 lit_color, float3 base_color, float3 ray_start_pos, float3 surface_position)
{
    float2 ray_total = surface_position.xz - ray_start_pos.xz;
    float ray_distance_sq = dot(ray_total, ray_total);
    const float shadow_distance = 20000.f;
    if (ray_distance_sq < shadow_distance * shadow_distance)
    {
        Raymarch_Result shadow_result = raymarch(surface_position + sun_direction * 10, sun_direction);
        if (shadow_result.hit)
        {
            float3 diffuse_color = albedo * base_color * One_Over_Pi;
            return ambient_color * diffuse_color;
        }
    }
    
    return lit_color;
}

float3 get_terrain_color(float3 position, float3 ray_start_pos, float3 norm_ray_direction)
{
    // base color
    const float base_height = 500;
    float3 base_color = 0.5 * lerp(float3(1, 1, 0.2), float3(0.7, 0.7, 0.7), smoothstep(-base_height, base_height, clamp(-position.y - 200, -base_height, base_height)));
        
    // normals
    float2 grad = bilinear_gradient_map(position.xz);
    float3 normal = normalize(float3(grad.x, -1, grad.y)); // beacause y is down gradients end up facing in the direction of greatest descent, which is why normals end up being correct
        
    Raymarch_Info raymarch_info = push_constants.raymarch_info.Get();
    if (raymarch_info.render_type == Render_Type_Normals)
    {
        normal.y *= -1;
        normal *= 0.5;
        normal += 0.5;
        return float4(normal, 1);
    }
        
    const float shininess = 70;
    float3 color = diffuse_specular(norm_ray_direction, base_color, normal, shininess);
    color = apply_shadow(color, base_color, ray_start_pos, position);
    
    return color;
}

float3 get_sky_color(float3 norm_ray_direction)
{
    float t = norm_ray_direction.y * 0.5f + 0.5f;
    float3 sky_color = lerp(float3(0.6f, 0.6f, 0.9f), float3(0.1f, 0.1f, 0.2f), t);
    
    const float sun_size = 0.999f;
    float s = max(dot(norm_ray_direction, sun_direction), sun_size);
    
    return lerp(sky_color, sun_color, smoothstep(sun_size, 1.f, s));
}

float4 get_color_for_ray(float3 ray_start_pos, float3 norm_ray_direction)
{
    Raymarch_Result result = raymarch(ray_start_pos, norm_ray_direction);
    if (result.hit)
    {
        float3 color = get_terrain_color(result.position, ray_start_pos, norm_ray_direction);
        
        // water
        if (result.position.y > 0) // y is down
        {
            float3 point_on_water_plane = float3(result.position.x, 0, result.position.z);
            const float3 water_normal = float3(0, -1, 0);
            
            float ray_water_cosine = dot(norm_ray_direction, water_normal);
            float t = dot(point_on_water_plane - ray_start_pos, water_normal) / ray_water_cosine; // ray is guaranteed to have intersection here
            float3 water_position = ray_start_pos + t * norm_ray_direction;
            
            float3 reflection_direction = norm_ray_direction - 2 * ray_water_cosine * water_normal;
            float reflection_factor = clamp(-ray_water_cosine, 0, 0.5);
            
            Raymarch_Result reflect_result = raymarch(water_position, reflection_direction);
            float3 reflected_color = reflect_result.hit ? get_terrain_color(reflect_result.position, ray_start_pos, reflection_direction) : get_sky_color(reflection_direction);
            
            const float3 base_color = float3(0.1, 0.1, 1);
            const float shininess = 5;
            float3 water_color = diffuse_specular(norm_ray_direction, base_color, water_normal, shininess);
            water_color = apply_shadow(water_color, base_color, ray_start_pos, water_position);
            
            water_color = lerp(water_color, reflected_color,  reflection_factor);
            const float water_alpha = 0.2;
            color = lerp(color, water_color, water_alpha);
        }
        
        return float4(color, 1);
    }
    
    return float4(get_sky_color(norm_ray_direction), 1);
}

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
    
    float4 color = get_color_for_ray(camera_info.positon, world_ray_direction).bgra; // swizzle to match actual bgra layout
    out_image[dispatch_thread_id.xy] = float4(pow(color.rgb, 0.4545), 1.f); // gamma correction
}
