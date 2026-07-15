#define Grad_Table_Size 16

static const float2 grad_table[Grad_Table_Size] =
{
    float2( 1,  1) / 1.414213562373095f,
    float2(-1,  1) / 1.414213562373095f,
    float2( 1, -1) / 1.414213562373095f,
    float2(-1, -1) / 1.414213562373095f,
    float2( 1,  0),
    float2(-1,  0),
    float2( 0,  1),
    float2( 0, -1),
    float2( 0.009138395397176f,  0.999958243993000f), // sin(     pi / 6) , cos(     pi / 6)
    float2( 0.018276027628547f,  0.999832979459129f), // sin(     pi / 3) , cos(     pi / 3)
    float2( 0.009138395397176f, -0.999958243993000f), // sin( 5 * pi / 6) , cos( 5 * pi / 6)
    float2( 0.018276027628547f, -0.999832979459129f), // sin( 2 * pi / 3) , cos( 2 * pi / 3)
    float2(-0.009138395397176f,  0.999958243993000f), // sin(   - pi / 6) , cos(   - pi / 6)
    float2(-0.018276027628547f,  0.999832979459129f), // sin(   - pi / 3) , cos(   - pi / 3)
    float2(-0.009138395397176f, -0.999958243993000f), // sin(-5 * pi / 6) , cos(-5 * pi / 6)
    float2(-0.018276027628547f, -0.999832979459129f), // sin(-2 * pi / 3) , cos(-2 * pi / 3)
};

/*
 * Vibe coded pcg-like seeded hash function
*/
inline uint permute_grad_index(uint index, uint64_t seed)
{
    uint64_t state = seed ^ (((uint64_t) index << 32) | index);
    
    state ^= state >> 33;
    state *= 0xff51afd7ed558ccdULL;
    state ^= state >> 33;
    state *= 0xc4ceb9fe1a85ec53ULL;
    state ^= state >> 33;
    
    uint oldstate = (uint) (state & 0xFFFFFFFFu);
    uint count = (uint) (state >> 59u);
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) * 277803737u;
    uint random_uint = (xorshifted >> 28u) | (xorshifted << ((32u - count) & 31u));
    
    return random_uint;
}

inline uint3 grad_indices(uint2 cell_coords_quare_grid, uint2 cell_1_offset_sq_grid, uint64_t seed)
{
    uint3 grad_indices;
    grad_indices.x = float(permute_grad_index(cell_coords_quare_grid.x + permute_grad_index(cell_coords_quare_grid.y, seed), seed) & (Grad_Table_Size - 1));
    grad_indices.y = float(permute_grad_index(cell_coords_quare_grid.x + cell_1_offset_sq_grid.x + permute_grad_index(cell_coords_quare_grid.y + cell_1_offset_sq_grid.y, seed), seed) & (Grad_Table_Size - 1));
    grad_indices.z = float(permute_grad_index(cell_coords_quare_grid.x + 1 + permute_grad_index(cell_coords_quare_grid.y + 1, seed), seed) & (Grad_Table_Size - 1));
    
    return grad_indices;
}

#define F 0.366025403784
#define G 0.211324865405

inline float2 skew(float2 p)
{
    return p + F * (p.x + p.y);
}

inline float2 unskew(float2 p)
{
    return p - G * (p.x + p.y);
}

/*
 * Implementation is based on:
 *    - https://www.researchgate.net/publication/216813608_Simplex_noise_demystified
 *    - https://mini.gmshaders.com/p/noise3
*/
float simplex_noise_2d(float2 p, uint64_t seed)
{
    float2 p_square_grid = skew(p);
    float2 cell_coords_sq_grid = floor(p_square_grid);
    float2 in_cell_offset_sq_grid = p_square_grid - cell_coords_sq_grid;
    uint2  cell_1_offset_sq_grid = in_cell_offset_sq_grid.x > in_cell_offset_sq_grid.y ? uint2(1, 0) : uint2(0, 1);
    
    float2 p_to_cell_0 = unskew(cell_coords_sq_grid) - p;
    float2 p_to_cell_1 = unskew(cell_coords_sq_grid + float2(cell_1_offset_sq_grid)) - p;
    float2 p_to_cell_2 = unskew(cell_coords_sq_grid + 1) - p;
    
    float3 square_distances;
    square_distances.x = dot(p_to_cell_0, p_to_cell_0);
    square_distances.y = dot(p_to_cell_1, p_to_cell_1);
    square_distances.z = dot(p_to_cell_2, p_to_cell_2);
    
    float3 weights = max(0.5 - square_distances, 0);
    weights = weights * weights * weights * weights;
    
    uint3 grad_idx = grad_indices(asuint(int2(cell_coords_sq_grid)), cell_1_offset_sq_grid, seed);
    float3 grad_dots;
    grad_dots.x = dot(grad_table[grad_idx.x], p_to_cell_0);
    grad_dots.y = dot(grad_table[grad_idx.y], p_to_cell_1);
    grad_dots.z = dot(grad_table[grad_idx.z], p_to_cell_2);
    
    return dot(weights, grad_dots) * 380; // [-1; 1]
}

RWStructuredBuffer<float> height_map[MAX_DESCRIPTOR_ARRAY_SIZE] : register(u0, space0);

struct Terrain_Generation_Settings
{
    uint terrain_size;
    float frequency;
    float amplitude;
    uint octave_count;
    uint render_distance;
    uint64_t seed;
};

struct Push_Constants
{
    vk::BufferPointer<Terrain_Generation_Settings> terrain_gen_settings;
    uint64_t octave_weights_buffer_address;
};

[[vk::push_constant]]
Push_Constants push_constants;

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    Terrain_Generation_Settings terrain_generation_settings = push_constants.terrain_gen_settings.Get();
    
    uint terrain_size = terrain_generation_settings.terrain_size;
    float frequency = terrain_generation_settings.frequency;
    float amplitude = terrain_generation_settings.amplitude;
    uint octave_count = terrain_generation_settings.octave_count;
    uint render_distance = terrain_generation_settings.render_distance;
    uint64_t seed = terrain_generation_settings.seed;
    
    float noise_value = 0;
    for (int octave = 1; octave <= octave_count; ++octave)
    {
        float octave_weight = vk::RawBufferLoad<float>(push_constants.octave_weights_buffer_address + (sizeof(float) * (octave - 1))); // maybe a bit hacky but I did not want to bother with descriptor sets
        
        int2 position = int2(dispatch_thread_id.xy) - int(terrain_size) / 2;
        position += int2(dispatch_thread_id.z / render_distance, dispatch_thread_id.z % render_distance) * int(terrain_size); // adjust for chunk position
        
        noise_value += simplex_noise_2d(float2(position) * frequency * float(octave), seed) * octave_weight;
    }
    height_map[dispatch_thread_id.z][dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = noise_value * amplitude;
}
