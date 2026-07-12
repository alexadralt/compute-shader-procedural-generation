RWStructuredBuffer<float>  height_map            : register(u0, space0);
RWStructuredBuffer<float2> out_norm_gradient_map : register(u1, space0);

struct Push_Constants
{
    uint terrain_size;
};

[[vk::push_constant]]
Push_Constants push_constants;

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint terrain_size = push_constants.terrain_size;
    
    float xy        = height_map[dispatch_thread_id.x * terrain_size + dispatch_thread_id.y];
    float x_plus_h  = dispatch_thread_id.x < terrain_size - 1 ? height_map[(dispatch_thread_id.x + 1) * terrain_size + dispatch_thread_id.y    ] : xy;
    float x_minus_h = dispatch_thread_id.x > 0                ? height_map[(dispatch_thread_id.x - 1) * terrain_size + dispatch_thread_id.y    ] : xy;
    float y_plus_h  = dispatch_thread_id.y < terrain_size - 1 ? height_map[ dispatch_thread_id.x      * terrain_size + dispatch_thread_id.y + 1] : xy;
    float y_minus_h = dispatch_thread_id.y > 0                ? height_map[ dispatch_thread_id.x      * terrain_size + dispatch_thread_id.y - 1] : xy;
    float2 grad = normalize(float2(x_plus_h - x_minus_h, y_plus_h - y_minus_h)); // formally, here we should divide by 2*h, but beacuse we normalize it gets cancelled out anyway
    
    out_norm_gradient_map[dispatch_thread_id.x * terrain_size + dispatch_thread_id.y] = grad;
}
