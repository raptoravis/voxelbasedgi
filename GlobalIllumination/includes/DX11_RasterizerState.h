#ifndef DX11_RASTERIZER_STATE_H
#define DX11_RASTERIZER_STATE_H

#include <render_states.h>

// descriptor for setting up DX11_RasterizerState
struct RasterizerDesc
{
	RasterizerDesc():
    fillMode(SOLID_FILL),
    cullMode(NONE_CULL),
    scissorTest(false),
    multisampleEnable(false)
	{
	}

	bool operator== (const RasterizerDesc &desc) const
	{
		if(fillMode != desc.fillMode)
			return false;
		if(cullMode != desc.cullMode)
			return false;
		if(scissorTest != desc.scissorTest)
			return false;
		if(multisampleEnable != desc.multisampleEnable)
			return false;
		return true;
	}

	fillModes fillMode;
	cullModes cullMode;
	bool scissorTest;
	bool multisampleEnable;
};

// DX11_RasterizerState
//
// Wrapper for ID3D11RasterizerState.
class DX11_RasterizerState
{
public:
  DX11_RasterizerState():
    rasterizerState(NULL)
	{
	}

  ~DX11_RasterizerState()
	{
		Release();
	}

	void Release();

	bool Create(const RasterizerDesc &desc);

	void Set() const;

	const RasterizerDesc& GetDesc() const
	{
		return desc;
	}

private:
	RasterizerDesc desc;
  ID3D11RasterizerState *rasterizerState;

};

#endif