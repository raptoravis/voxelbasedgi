#ifndef DX11_SAMPLER_H
#define DX11_SAMPLER_H

#include <render_states.h>

// descriptor for setting up DX11_Sampler
struct SamplerDesc
{
	SamplerDesc() :
		filter(MIN_MAG_LINEAR_FILTER),
		maxAnisotropy(2),
		adressU(CLAMP_TEX_ADRESS),
		adressV(CLAMP_TEX_ADRESS),
		adressW(CLAMP_TEX_ADRESS),
		minLOD(0.0f),
		maxLOD(FLT_MAX),
		lodBias(0.0f),
		compareFunc(LEQUAL_COMP_FUNC)
	{
	}

	bool operator== (const SamplerDesc &desc) const
	{
		if (filter != desc.filter)
			return false;
		if (maxAnisotropy != desc.maxAnisotropy)
			return false;
		if (adressU != desc.adressU)
			return false;
		if (adressV != desc.adressV)
			return false;
		if (adressW != desc.adressW)
			return false;
		if (borderColor != desc.borderColor)
			return false;
		if (!IS_EQUAL(minLOD, desc.minLOD))
			return false;
		if (!IS_EQUAL(maxLOD, desc.maxLOD))
			return false;
		if (!IS_EQUAL(lodBias, desc.lodBias))
			return false;
		if (compareFunc != desc.compareFunc)
			return false;
		return true;
	}

	filterModes filter;
	unsigned int maxAnisotropy;
	texAdressModes adressU;
	texAdressModes adressV;
	texAdressModes adressW;
	Color borderColor;
	float minLOD;
	float maxLOD;
	float lodBias;
	comparisonFuncs compareFunc;
};

// DX11_Sampler
//
// Wrapper for ID3D11SamplerState.
class DX11_Sampler
{
public:
	DX11_Sampler() :
		sampler(NULL)
	{
	}

	~DX11_Sampler()
	{
		Release();
	}

	void Release();

	bool Create(const SamplerDesc &desc);

	void Bind(textureBP bindingPoint, shaderTypes shaderType = VERTEX_SHADER) const;

	const SamplerDesc& GetDesc() const
	{
		return desc;
	}

private:
	SamplerDesc desc;
	ID3D11SamplerState *sampler;

};

#endif