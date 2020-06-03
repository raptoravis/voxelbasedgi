#include "globals.shi"

struct VS_Output
{
	float4 position: SV_POSITION;
};

struct FS_Output
{
  float4 fragColor: SV_TARGET;
};

#define SKY_COLOR float4(0.05f, 0.21f, 0.83f, 1.0f)

FS_Output main(VS_Output input) 
{
  FS_Output output;
	output.fragColor = SKY_COLOR;
	return output;
}