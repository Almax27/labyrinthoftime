// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
	float4 ambientColor;
    float4 diffuseColor;
    float3 lightPosition;
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
	float3 position3D : TEXCOORD2;
};

float4 main(InputType input) : SV_TARGET
{
	float4	textureColor;
    float3	lightDir;
    float	lightIntensity;
    float4	color;
	float4  heightColor;

	// Invert the light direction for calculations.
	lightDir = normalize(input.position3D - lightPosition);

	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(input.normal, -lightDir));

	// Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
	color = ambientColor + (diffuseColor * lightIntensity); //adding ambient
	color = saturate(color);

	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor = shaderTexture.Sample(SampleType, input.tex);
	if (input.position3D.y <= 1.0 && input.position3D.y > 0.75)
	{
		heightColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	else if (input.position3D.y < 0.75 && input.position3D.y > 0.5)
	{
		heightColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if (input.position3D.y < 0.5 && input.position3D.y > 0.25)
	{
		heightColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (input.position3D.y < 0.25 && input.position3D.y >= 0)
	{
		heightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (input.position3D.y > 1.0) 
	{
		heightColor = float4(1.0f, 0.65f, 0.0f, 1.0f);
	}
	color = color * heightColor * textureColor;

    return color;
}

