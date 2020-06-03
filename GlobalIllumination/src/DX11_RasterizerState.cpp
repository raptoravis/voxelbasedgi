#include <stdafx.h>
#include <Demo.h>
#include <DX11_RasterizerState.h>

void DX11_RasterizerState::Release()
{
	SAFE_RELEASE(rasterizerState);
}

bool DX11_RasterizerState::Create(const RasterizerDesc &desc)
{
	this->desc = desc;
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = (D3D11_FILL_MODE)desc.fillMode;
	rasterDesc.CullMode = (D3D11_CULL_MODE)desc.cullMode;
	rasterDesc.FrontCounterClockwise = TRUE;
	rasterDesc.ScissorEnable = desc.scissorTest;
  rasterDesc.MultisampleEnable = desc.multisampleEnable;

	if(Demo::renderer->GetDevice()->CreateRasterizerState(&rasterDesc, &rasterizerState) != S_OK)
		return false;

	return true;
}

void DX11_RasterizerState::Set() const
{
	Demo::renderer->GetDeviceContext()->RSSetState(rasterizerState);
}


