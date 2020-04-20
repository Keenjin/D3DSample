float4 VS( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}

float4 PS() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

// Technique Definition
technique10 Render
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
