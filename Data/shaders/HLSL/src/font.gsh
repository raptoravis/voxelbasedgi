struct VS_Output
{
	float3 position: POSITION;
  float2 texCoords: TEXCOORD;
	float4 color: COLOR;
};

struct GS_Output
{
  float4 position: SV_POSITION;
  float2 texCoords: TEXCOORD;
	float4 color: COLOR;
};

[maxvertexcount(4)]
void main(line VS_Output input[2], inout TriangleStream<GS_Output> outputStream)
{ 
   // generate a quad from input line (2 vertices)
	// ->generate 1 triangle-strip
	GS_Output output;
	
  // left/ lower vertex
	output.position = float4(input[0].position.x, input[0].position.y, input[0].position.z, 1.0f);
	output.texCoords = float2(input[0].texCoords.x, input[0].texCoords.y);
	output.color = input[0].color;
  outputStream.Append(output);
	
	// right/ lower vertex
	output.position = float4(input[1].position.x, input[0].position.y, input[0].position.z, 1.0f);
	output.texCoords = float2(input[1].texCoords.x, input[0].texCoords.y);
	output.color = input[1].color;
  outputStream.Append(output);

	// left/ upper vertex
	output.position = float4(input[0].position.x, input[1].position.y, input[0].position.z, 1.0f);
	output.texCoords = float2(input[0].texCoords.x, input[1].texCoords.y);
  output.color = input[0].color;
  outputStream.Append(output);

  // right/ upper vertex
	output.position = float4(input[1].position.x, input[1].position.y, input[0].position.z, 1.0f);
	output.texCoords = float2(input[1].texCoords.x, input[1].texCoords.y);
	output.color = input[1].color;
  outputStream.Append(output);

	outputStream.RestartStrip();
}