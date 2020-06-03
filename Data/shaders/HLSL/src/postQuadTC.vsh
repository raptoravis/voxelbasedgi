struct VS_Output
{
	float4 position: SV_POSITION;
  float2 texCoords: TEXCOORD;
};

static const float4 positionTexCoords[3] =
{
  float4(-1.0f, -1.0f, 0.0f, 1.0f),
  float4(3.0f, -1.0f, 2.0f, 1.0f),
  float4(-1.0f, 3.0f, 0.0f, -1.0f) 
};

VS_Output main(uint vertexID: SV_VertexID)
{
  VS_Output output;
  float4 outputPositionTexCoords = positionTexCoords[vertexID];
	output.position = float4(outputPositionTexCoords.xy, 0.0f, 1.0f);
	output.texCoords = outputPositionTexCoords.zw;
  return output;
}
