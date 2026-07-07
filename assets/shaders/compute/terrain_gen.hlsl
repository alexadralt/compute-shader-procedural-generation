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

inline float smax(float x, float y, float lambda)
{
    return (x + y + sqrt((x - y) * (x - y) + lambda)) / 2;
}

#define Grad_Index_Table_Size 256

static const uint grad_index_table[Grad_Index_Table_Size * 2] =
{
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
    77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
    135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
    223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
    251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
    49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 18,
    
    // duplicate to avoid index wrapping
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
    77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
    135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
    223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
    251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
    49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 18
};

#define Grad_Table_Size 12

static const float2 grad_table[Grad_Table_Size] =
{
    float2( 1,  1),
    float2(-1,  1),
    float2( 1, -1),
    float2(-1, -1),
    float2( 1,  0),
    float2(-1,  0),
    float2( 1,  0),
    float2(-1,  0),
    float2( 0,  1),
    float2( 0, -1),
    float2( 0,  1),
    float2( 0, -1)
};

inline uint3 grad_indices(uint2 cell_coords_quare_grid, uint2 cell_1_offset_sq_grid)
{
    cell_coords_quare_grid &= Grad_Index_Table_Size - 1;
    
    uint3 grad_indices;
    grad_indices.x = float(grad_index_table[cell_coords_quare_grid.x + grad_index_table[cell_coords_quare_grid.y]] % Grad_Table_Size);
    grad_indices.y = float(grad_index_table[cell_coords_quare_grid.x + cell_1_offset_sq_grid.x + grad_index_table[cell_coords_quare_grid.y + cell_1_offset_sq_grid.y]] % Grad_Table_Size);
    grad_indices.z = float(grad_index_table[cell_coords_quare_grid.x + 1 + grad_index_table[cell_coords_quare_grid.y + 1]] % Grad_Table_Size);
    
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

float simplex_noise_2d(float2 p)
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
    
    uint3 grad_idx = grad_indices(uint2(cell_coords_sq_grid), cell_1_offset_sq_grid);
    float3 grad_dots;
    grad_dots.x = dot(grad_table[grad_idx.x], p_to_cell_0);
    grad_dots.y = dot(grad_table[grad_idx.y], p_to_cell_1);
    grad_dots.z = dot(grad_table[grad_idx.z], p_to_cell_2);
    
    return dot(weights, grad_dots) * 70; // [-1; 1]
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    Terrain_Generation_Settings terrain_generation_settings = push_constants.terrain_gen_settings.Get();
    
    uint terrain_size = terrain_generation_settings.terrain_size;
    float frequency = terrain_generation_settings.frequency;
    float amplitude = terrain_generation_settings.amplitude;
    
    float noise_value = simplex_noise_2d(float2(dispatch_thread_id.xy) * frequency);
    height_map[dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = noise_value * amplitude;
    
    float scaled_noise_value = (noise_value + 1.0) / 2.0;
    float4 final_color = float4(max(0.5 - scaled_noise_value, 0.0) * 2.0, 0, max(scaled_noise_value - 0.5, 0.0) * 2.0, 1);
    height_map_image[uint2(dispatch_thread_id.x, dispatch_thread_id.y)] = final_color.bgra; // swizzle to match actual bgra layout
}
