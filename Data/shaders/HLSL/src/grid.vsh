struct VS_Output
{
	float2 position: POSITION;
	uint instanceID: INSTANCE_ID;
};

static const float2 positions[3] =
{
  float2(-1.0f, -1.0f),
  float2(3.0f, -1.0f),
  float2(-1.0f, 3.0f) 
};

VS_Output main(uint vertexID: SV_VertexID, uint instanceID: SV_InstanceID)
{
  VS_Output output;
	output.position = positions[vertexID];
	output.instanceID = instanceID;
  return output;
}
