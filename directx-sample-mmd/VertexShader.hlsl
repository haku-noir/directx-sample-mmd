#include "Header.hlsli"

Vertex main(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneNo : BONE_NO,
	min16uint weight : WEIGHT
){
	Vertex output;
	output.svpos = mul(mat, pos);
	return output;
}