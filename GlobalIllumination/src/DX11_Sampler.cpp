#include <stdafx.h>
#include <Demo.h>
#include <DX11_Sampler.h>

void DX11_Sampler::Release()
{
	SAFE_RELEASE(sampler);
}

bool DX11_Sampler::Create(const SamplerDesc &desc)
{
	this->desc = desc;
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = (D3D11_FILTER)desc.filter;
	samplerDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)desc.adressU;
	samplerDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)desc.adressV;
	samplerDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)desc.adressW;
	samplerDesc.MipLODBias = desc.lodBias;
	samplerDesc.MaxAnisotropy = desc.maxAnisotropy;
	samplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)desc.compareFunc;
	memcpy(samplerDesc.BorderColor, desc.borderColor, sizeof(Color));
	samplerDesc.MinLOD = desc.minLOD;
	samplerDesc.MaxLOD = desc.maxLOD;

	if(Demo::renderer->GetDevice()->CreateSamplerState(&samplerDesc, &sampler) != S_OK)
		return false;
	
	return true;
}

void DX11_Sampler::Bind(textureBP bindingPoint, shaderTypes shaderType) const
{
	switch(shaderType)
	{
	case VERTEX_SHADER:
		Demo::renderer->GetDeviceContext()->VSSetSamplers(bindingPoint, 1, &sampler);
		break;

	case GEOMETRY_SHADER:
		Demo::renderer->GetDeviceContext()->GSSetSamplers(bindingPoint, 1, &sampler);
		break;

	case FRAGMENT_SHADER:
    Demo::renderer->GetDeviceContext()->PSSetSamplers(bindingPoint, 1, &sampler);
		break;

	case COMPUTE_SHADER:
		Demo::renderer->GetDeviceContext()->CSSetSamplers(bindingPoint, 1, &sampler);
		break;
	}	
}


