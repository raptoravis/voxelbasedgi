#include <stdafx.h>
#include <Demo.h>
#include <DeferredLighting.h>

void DeferredLighting::Execute()
{
	if(!active)
		return;
	for(unsigned int i=0; i<Demo::renderer->GetNumLights(); i++) 
    Demo::renderer->GetLight(i)->AddLitSurface();
}
