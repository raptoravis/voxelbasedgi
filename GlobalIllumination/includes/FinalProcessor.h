#ifndef FINAL_PROCESSOR_H
#define FINAL_PROCESSOR_H

#include <IPostProcessor.h>

class DX11_RenderTarget;
class DX11_Shader;

// FinalProcessor
//
// Copies content of the accumulation buffer (of the GBuffers) into the back buffer
// while performing tone-mapping.
class FinalProcessor : public IPostProcessor
{
public:
	FinalProcessor() :
		sceneRT(NULL),
		backBufferRT(NULL),
		finalPassShader(NULL)
	{
		strcpy(name, "FinalProcessor");
	}

	virtual bool Create() override;

	virtual DX11_RenderTarget* GetOutputRT() const override
	{
		return backBufferRT;
	}

	virtual void Execute() override;

private:
	DX11_RenderTarget *sceneRT;
	DX11_RenderTarget *backBufferRT;
	DX11_Shader *finalPassShader;

};

#endif