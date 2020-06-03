#ifndef SKY_H
#define SKY_H

#include <IPostProcessor.h>

class DX11_RenderTarget;
class DX11_RenderTargetConfig;
class DX11_Shader;
class DX11_DepthStencilState;

// Sky
//
// Extremely simple sky post-processor. Since all previously rendered opaque geometry  
// had incremented the stencil buffer, for the sky a constant colored full-screen quad 
// is only rendered where the stencil buffer is still 0.
class Sky: public IPostProcessor
{
public: 
	Sky():
    sceneRT(NULL),
    rtConfig(NULL),
    skyShader(NULL),
    depthStencilState(NULL)
	{
		strcpy(name, "Sky");
	}

	virtual bool Create() override;

	virtual DX11_RenderTarget* GetOutputRT() const override
	{
		return sceneRT;
	}

	virtual void Execute() override;

private:
	DX11_RenderTarget *sceneRT;
  DX11_RenderTargetConfig *rtConfig;
	DX11_Shader *skyShader;
	DX11_DepthStencilState *depthStencilState;

};

#endif