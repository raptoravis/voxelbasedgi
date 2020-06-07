#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include <ILight.h>

class DX11_RasterizerState;
class DX11_DepthStencilState;
class DX11_BlendState;
class DX11_RenderTargetConfig;
class DX11_Shader;

// DirectionalLight
//
// For direct illumination a full-screen quad is rendered deferred into the accumulation render-target 
// of the GBuffers. In order to cast shadows a shadow map is generated. For indirect illumination the 
// voxel-grid of the GlobalIllum post-processor is illuminated, whereby the shadow map, that was generated
// for direct illumination, is reused.
class DirectionalLight : public ILight
{
public:
	struct BufferData
	{
		BufferData() :
			multiplier(0.0f),
			invShadowMapSize(0.0f)
		{
		}

		Vector3 direction;
		float multiplier;
		Color color;
		Matrix4 shadowViewProjMatrix;
		Matrix4 shadowViewProjTexMatrix;
		float invShadowMapSize;
	};

	DirectionalLight() :
		lightShader(NULL),
		shadowMapShader(NULL),
		uniformBuffer(NULL),
		noneCullRS(NULL),
		backCullRS(NULL),
		defaultDSS(NULL),
		noDepthTestDSS(NULL),
		noColorBS(NULL),
		blendBS(NULL),
		rtConfig(NULL),
		frustumRadius(0.0f),
		frustumRatio(0.0f)
	{
		lightGridShaders[FINE_GRID] = NULL;
		lightGridShaders[COARSE_GRID] = NULL;
	}

	bool Create(const Vector3 &direction, const Color &color, float multiplier);

	virtual lightTypes GetLightType() const override
	{
		return DIRECTIONAL_LT;
	}

	virtual void Update() override;

	virtual void SetupShadowMapSurface(DrawCmd &drawCmd) override;

	virtual void AddLitSurface() override;

	virtual void AddGridSurfaces() override;

	virtual DX11_UniformBuffer* GetUniformBuffer() const override
	{
		return uniformBuffer;
	}

	void SetDirection(const Vector3 &direction)
	{
		bufferData.direction = direction.GetNormalized();
	}

	Vector3 GetDirection() const
	{
		return bufferData.direction;
	}

	void SetColor(const Color &color)
	{
		bufferData.color = color;
	}

	Color GetColor() const
	{
		return bufferData.color;
	}

	void SetMultiplier(float multiplier)
	{
		bufferData.multiplier = multiplier;
	}

	float GetMultiplier() const
	{
		return bufferData.multiplier;
	}

private:
	void CalculateFrustum();

	void CalculateMatrices();

	void UpdateUniformBuffer();

	// data for directional light uniform-buffer
	BufferData bufferData;

	DX11_Shader *lightShader;
	DX11_Shader *shadowMapShader;
	DX11_Shader *lightGridShaders[2];
	DX11_UniformBuffer *uniformBuffer;
	DX11_RasterizerState *noneCullRS;
	DX11_RasterizerState *backCullRS;
	DX11_DepthStencilState *defaultDSS;
	DX11_DepthStencilState *noDepthTestDSS;
	DX11_BlendState *noColorBS;
	DX11_BlendState *blendBS;
	DX11_RenderTargetConfig *rtConfig;

	Matrix4 shadowTexMatrix;
	Matrix4 shadowProjMatrix;
	float frustumRadius;
	float frustumRatio;

};

#endif
