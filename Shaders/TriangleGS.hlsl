// TriangleGS.hlsl
// Geometry shader that takes a point and outputs a triangle.

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION; // Expecting position from Vertex Shader (though we won't use it directly)
};

struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

[maxvertexcount(3)]
void main(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> triStream)
{
    GS_OUTPUT output;

    // Define a simple triangle in Normalized Device Coordinates (NDC)
    // The Z coordinate can be 0.0 (near plane) or 0.5, W should be 1.0

    // Vertex 1
    output.Pos = float4(-0.5f, -0.5f, 0.5f, 1.0f); // Bottom-left
    output.Color = float4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    triStream.Append(output);

    // Vertex 2
    output.Pos = float4(0.0f, 0.5f, 0.5f, 1.0f);  // Top-middle
    output.Color = float4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    triStream.Append(output);

    // Vertex 3
    output.Pos = float4(0.5f, -0.5f, 0.5f, 1.0f); // Bottom-right
    output.Color = float4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    triStream.Append(output);

    triStream.RestartStrip();
}
