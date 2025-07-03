// ClosestHit.hlsl
struct Attributes
{
    float2 barycentrics : SV_IntersectionAttributes;
};

struct RayPayload
{
    float4 color : SV_RayPayload;
    uint recursionDepth : SV_RayPayload;
};

// Example vertex structure (adjust if different)
struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
};

// Assuming triangles for now
//StructuredBuffer<Vertex> vertices : register(t1, space0); // Example: if you bind vertex buffer
//ByteAddressBuffer indices : register(t2, space0);       // Example: if you bind index buffer


[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in Attributes attr)
{
    // Simple: return a fixed color
    payload.color = float4(0.8f, 0.8f, 0.8f, 1.0f); // Grey

    // More advanced: Could try to get primitive index and use it
    // uint primitiveIdx = PrimitiveIndex();
    // uint instanceIdx = InstanceIndex();
    // float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Example: Accessing vertex data (requires binding vertex/index buffers and knowing format)
    // This is more complex and will be done in a later step.
    /*
    uint3 triangleIndices = indices.Load3(PrimitiveIndex() * 3 * sizeof(uint));
    Vertex v0 = vertices[triangleIndices.x];
    Vertex v1 = vertices[triangleIndices.y];
    Vertex v2 = vertices[triangleIndices.z];

    float3 hitNormal = v0.Normal * (1.0f - attr.barycentrics.x - attr.barycentrics.y) +
                       v1.Normal * attr.barycentrics.x +
                       v2.Normal * attr.barycentrics.y;
    hitNormal = normalize(hitNormal);
    payload.color = float4(abs(hitNormal), 1.0f);
    */
}
