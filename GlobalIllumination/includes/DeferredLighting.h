#ifndef DEFERRED_LIGHTING_H
#define DEFERRED_LIGHTING_H

#include <IPostProcessor.h>

// DeferredLighting
//
// Performs deferred lighting.
class DeferredLighting : public IPostProcessor
{
public:
	DeferredLighting()
	{
		strcpy(name, "DeferredLighting");
	}

	virtual bool Create() override
	{
		return true;
	}

	virtual DX11_RenderTarget* GetOutputRT() const override
	{
		return NULL;
	}

	virtual void Execute() override;

};

#endif