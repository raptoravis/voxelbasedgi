#ifndef ILIGHT_H
#define ILIGHT_H

#include <GlobalIllum.h>

enum lightTypes
{
	POINT_LT=0,
	DIRECTIONAL_LT
};

class GpuCmd;
class DX11_UniformBuffer;

// ILight
//
// Interface for different light types.
class ILight
{
public:	
	ILight():
    index(0), 
    globalIllumPP(NULL),
    active(true),
    hasShadow(false),
    performUpdate(true)
	{
	}

	virtual ~ILight()
	{
	}

	virtual lightTypes GetLightType() const=0;

	virtual void Update()=0;

	virtual void SetupShadowMapSurface(DrawCmd &drawCmd)=0;

	// adds surface for direct illumination
	virtual void AddLitSurface()=0;

	// adds surfaces for indirect illumination
	virtual void AddGridSurfaces()=0;

	virtual DX11_UniformBuffer* GetUniformBuffer() const=0;
	
	unsigned int GetIndex() const
	{
		return index;
	}

	void SetActive(bool active) 
	{
		this->active = active;
	}

	bool IsActive() const
	{
		return active;
	}

	bool HasShadow() const
	{
		return hasShadow;
	}

protected:
	unsigned int index;
  GlobalIllum *globalIllumPP;
	bool active; 
	bool hasShadow;
	bool performUpdate;
	
};

#endif
