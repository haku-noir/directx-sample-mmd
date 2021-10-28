#include "Header.hlsli"

float4 main( Vertex input ) : SV_TARGET
{
	return float4(input.normal.xyz, 1.0f);
}