RWBuffer<float> height_map : register(u0);
RWBuffer<float3> gradients : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    height_map[dispatch_thread_id.x * 5 + dispatch_thread_id.y] = 133.7;
}
