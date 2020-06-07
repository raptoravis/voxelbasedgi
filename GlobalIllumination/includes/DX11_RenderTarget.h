#ifndef DX11_RENDER_TARGET_H
#define DX11_RENDER_TARGET_H

#include <render_states.h>

// max number of color-buffers, which can be attached to 1 render-target
#define MAX_NUM_COLOR_BUFFERS 8

enum renderTargetFlags
{
	UAV_RTF = 1, // create unordered access view
	SRGB_RTF = 2 // convert color from linear to SRGB space
};

class DX11_Texture;
class DX11_Sampler;
class DX11_RenderTargetConfig;

// descriptor for a render-target buffer
struct RtBufferDesc
{
	RtBufferDesc() :
		format(TEX_FORMAT_NONE),
		rtFlags(0)
	{
	}

	texFormats format;
	unsigned int rtFlags;
};

// descriptor for setting up DX11_RenderTarget
struct RenderTargetDesc
{
	RenderTargetDesc() :
		width(0),
		height(0),
		depth(1),
		numLevels(1)
	{
	}

	unsigned int CalcNumColorBuffers() const
	{
		unsigned int numColorBuffers = 0;
		for (unsigned int i = 0; i < MAX_NUM_COLOR_BUFFERS; i++)
		{
			if (colorBufferDescs[i].format != TEX_FORMAT_NONE)
				numColorBuffers++;
			else
				break;
		}
		return numColorBuffers;
	}

	unsigned int width, height, depth;
	unsigned int numLevels;
	RtBufferDesc colorBufferDescs[MAX_NUM_COLOR_BUFFERS];
	RtBufferDesc depthStencilBufferDesc;
};

// DX11_RenderTarget
//
// Render-target to render/ write into. Can be configured via DX11_RenderTargetConfig for each draw-call/ dispatch.
class DX11_RenderTarget
{
public:
	DX11_RenderTarget() :
		width(0),
		height(0),
		depth(0),
		numColorBuffers(0),
		rtFlags(0),
		clearMask(0),
		renderTargetViews(NULL),
		frameBufferTextures(NULL),
		depthStencilTexture(NULL),
		clearTarget(true)
	{
		memset(depthStencilViews, 0, sizeof(ID3D11DepthStencilView*) * 2);
	}

	~DX11_RenderTarget()
	{
		Release();
	}

	void Release();

	bool Create(const RenderTargetDesc &desc);

	bool CreateBackBuffer();

	void Bind(const DX11_RenderTargetConfig *rtConfig = NULL);

	// indicate, that render-target should be cleared
	void Reset()
	{
		clearTarget = true;
	}

	void Clear(unsigned int newClearMask) const;

	DX11_Texture* GetTexture(unsigned int index = 0) const
	{
		assert(index < numColorBuffers);
		return &frameBufferTextures[index];
	}

	DX11_Texture* GetDepthStencilTexture() const
	{
		return depthStencilTexture;
	}

	unsigned int GetWidth() const
	{
		return width;
	}

	unsigned int GetHeight() const
	{
		return height;
	}

	unsigned int GetDepth() const
	{
		return depth;
	}

private:
	unsigned int width, height, depth;
	unsigned int numColorBuffers;
	unsigned int rtFlags;
	unsigned int clearMask;
	ID3D11RenderTargetView **renderTargetViews;
	DX11_Texture *frameBufferTextures;
	ID3D11DepthStencilView *depthStencilViews[2];
	DX11_Texture *depthStencilTexture;
	D3D11_VIEWPORT viewport;
	bool clearTarget;

};

#endif