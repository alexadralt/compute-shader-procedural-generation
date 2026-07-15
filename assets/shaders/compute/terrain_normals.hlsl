StructuredBuffer<float>    height_map[MAX_DESCRIPTOR_ARRAY_SIZE]            : register(t0, space0);
RWStructuredBuffer<float2> out_norm_gradient_map[MAX_DESCRIPTOR_ARRAY_SIZE] : register(u1, space0);

struct Push_Constants
{
    uint terrain_size;
    uint render_distance;
};

[[vk::push_constant]]
Push_Constants push_constants;

inline uint map_index(uint2 pos)
{
    uint2 wrapped = pos % push_constants.render_distance;
    return wrapped.x * push_constants.render_distance + wrapped.y;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint terrain_size = push_constants.terrain_size;
    uint render_distance = push_constants.render_distance;
    
    uint2 chunk_coords = uint2(dispatch_thread_id.z / render_distance, dispatch_thread_id.z % render_distance);
    
    uint x_plus_h_index  = map_index(chunk_coords + uint2(dispatch_thread_id.x + 1 >= terrain_size, 0));
    uint x_minus_h_index = map_index(chunk_coords - uint2(dispatch_thread_id.x     == 0,            0));
    uint y_plus_h_index  = map_index(chunk_coords + uint2(0, dispatch_thread_id.y + 1 >= terrain_size));
    uint y_minus_h_index = map_index(chunk_coords - uint2(0, dispatch_thread_id.y     == 0           ));
    
    float x_plus_h  = height_map[x_plus_h_index ][((dispatch_thread_id.x + 1) & (terrain_size - 1)) * terrain_size + dispatch_thread_id.y    ];
    float x_minus_h = height_map[x_minus_h_index][((dispatch_thread_id.x - 1) & (terrain_size - 1)) * terrain_size + dispatch_thread_id.y    ];
    float y_plus_h  = height_map[y_plus_h_index ][ dispatch_thread_id.x                             * terrain_size + ((dispatch_thread_id.y + 1) & (terrain_size - 1))];
    float y_minus_h = height_map[y_minus_h_index][ dispatch_thread_id.x                             * terrain_size + ((dispatch_thread_id.y - 1) & (terrain_size - 1))];
    float2 grad = normalize(float2(x_plus_h - x_minus_h, y_plus_h - y_minus_h)); // formally, here we should divide by 2*h, but beacuse we normalize it gets cancelled out anyway
    
    out_norm_gradient_map[dispatch_thread_id.z][dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = grad;
}
