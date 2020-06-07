#ifndef DX11_BLEND_STATE_H
#define DX11_BLEND_STATE_H

#include <render_states.h>

// descriptor for setting up DX11_BlendState
struct BlendDesc
{
	BlendDesc() :
		srcColorBlend(ONE_BLEND),
		dstColorBlend(ONE_BLEND),
		blendColorOp(ADD_BLEND_OP),
		srcAlphaBlend(ONE_BLEND),
		dstAlphaBlend(ONE_BLEND),
		blendAlphaOp(ADD_BLEND_OP),
		blend(false),
		colorMask(ALL_COLOR_MASK)
	{
	}

	bool operator== (const BlendDesc &desc) const
	{
		if (srcColorBlend != desc.srcColorBlend)
			return false;
		if (dstColorBlend != desc.dstColorBlend)
			return false;
		if (blendColorOp != desc.blendColorOp)
			return false;
		if (srcAlphaBlend != desc.srcAlphaBlend)
			return false;
		if (dstAlphaBlend != desc.dstAlphaBlend)
			return false;
		if (blendAlphaOp != desc.blendAlphaOp)
			return false;
		if (constBlendColor != desc.constBlendColor)
			return false;
		if (blend != desc.blend)
			return false;
		if (colorMask != desc.colorMask)
			return false;
		return true;
	}

	blendOptions srcColorBlend;
	blendOptions dstColorBlend;
	blendOps blendColorOp;
	blendOptions srcAlphaBlend;
	blendOptions dstAlphaBlend;
	blendOps blendAlphaOp;
	Color constBlendColor;
	bool blend;
	unsigned char colorMask;
};

// DX11_BlendState
//
// Wrapper for ID3D11BlendState.
class DX11_BlendState
{
public:
	DX11_BlendState() :
		blendState(NULL)
	{
	}

	~DX11_BlendState()
	{
		Release();
	}

	void Release();

	bool Create(const BlendDesc &desc);

	void Set() const;

	const BlendDesc& GetDesc() const
	{
		return desc;
	}

private:
	BlendDesc desc;
	ID3D11BlendState *blendState;

};

#endif