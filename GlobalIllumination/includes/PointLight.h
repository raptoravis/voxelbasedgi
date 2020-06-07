#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include <ILight.h>

class DX11_Shader;
class DX11_RasterizerState;
class DX11_DepthStencilState;
class DX11_BlendState;
class DX11_RenderTargetConfig;

// PointLight
//
// For direct illumination a sphere geometry is rendered deferred into the accumulation render-target
// of the GBuffers. For indirect illumination the voxel-grid of the GlobalIllum post-processor is 
// illuminated. 
// Since often for point and spot lights shadows can be abandoned without causing unpleasant visual 
// effects, in this demo point lights do not cast shadows. However especially for large point lights 
// shadow maps have to be used same as for directional lights. According to directional lights the 
// shadow maps, that have been generated for direct illumination, are reused for indirect illumination.
class PointLight : public ILight
{
public:
	struct BufferData
	{
		BufferData() :
			radius(0.0f),
			multiplier(0.0f)
		{
		}

		Vector3 position;
		float radius;
		Color color;
		Matrix4 worldMatrix;
		float multiplier;
	};

	PointLight() :
		lightShader(NULL),
		uniformBuffer(NULL),
		backCullRS(NULL),
		frontCullRS(NULL),
		noDepthWriteDSS(NULL),
		noDepthTestDSS(NULL),
		blendBS(NULL),
		rtConfig(NULL),
		cameraInVolume(false)
	{
		lightGridShaders[FINE_GRID] = NULL;
		lightGridShaders[COARSE_GRID] = NULL;
	}

	bool Create(const Vector3 &position, float radius, const Color &color, float multiplier);

	virtual lightTypes GetLightType() const override
	{
		return POINT_LT;
	}

	virtual void Update() override;

	virtual void SetupShadowMapSurface(DrawCmd &drawCmd) override
	{
	}

	virtual void AddLitSurface() override;

	virtual void AddGridSurfaces() override;

	virtual DX11_UniformBuffer* GetUniformBuffer() const override
	{
		return uniformBuffer;
	}

	void SetPosition(const Vector3 &position);

	Vector3 GetPosition() const
	{
		return bufferData.position;
	}

	void SetRadius(float radius);

	float GetRadius() const
	{
		return bufferData.radius;
	}

	void SetColor(const Color &color);

	Color GetColor() const
	{
		return bufferData.color;
	}

	void SetMultiplier(float multiplier);

	float GetMultiplier() const
	{
		return bufferData.multiplier;
	}

private:
	bool IsSphereInVolume(const Vector3 &position, float radius) const;

	void CalculateMatrices();

	void UpdateUniformBuffer();

	// data for point light uniform-buffer
	BufferData bufferData;

	DX11_Shader *lightShader;
	DX11_Shader *lightGridShaders[2];
	DX11_UniformBuffer *uniformBuffer;
	DX11_RasterizerState *backCullRS;
	DX11_RasterizerState *frontCullRS;
	DX11_DepthStencilState *noDepthWriteDSS;
	DX11_DepthStencilState *noDepthTestDSS;
	DX11_BlendState *blendBS;
	DX11_RenderTargetConfig *rtConfig;
	bool cameraInVolume;

};

#endif
