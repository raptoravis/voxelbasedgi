#ifndef APPLICATION_H
#define APPLICATION_H

#include <PathPointLight.h>

#define NUM_PATH_POINT_LIGHTS 12

class Camera;
class Font;
class GlobalIllum;
class DirectionalLight;
class DemoMesh;

// Application
//
// Demo application, that demonstrate the "Rasterized Voxel-based Dynamic Global Illumination" technique.
class Application
{
public:
	Application() :
		mainCamera(NULL),
		defaultFont(NULL),
		globalIllum(NULL),
		dirLight(NULL),
		dynamicMesh(NULL),
		quit(false),
		pathLigthsEnabled(true),
		pathLightsAnimated(true),
		showHelp(false),
		showSettings(false)
	{
	}

	bool Init();

	void Run();

	void Shutdown();

	void Quit();

	void EnablePathLights(bool enable);

	bool ArePathLightsEnabled() const
	{
		return pathLigthsEnabled;
	}

	void AnimatePathLights(bool animate);

	bool ArePathLightsAnimated() const
	{
		return pathLightsAnimated;
	}

private:
	bool OnInit();

	void OnRun();

	void HandleInput();

	void DisplayInfo();

	Camera *mainCamera;
	POINT prevMousePos;
	Font *defaultFont;
	GlobalIllum *globalIllum;
	DirectionalLight *dirLight;
	PathPointLight pathPointLights[NUM_PATH_POINT_LIGHTS];
	DemoMesh *dynamicMesh;
	bool quit;
	bool pathLigthsEnabled;
	bool pathLightsAnimated;
	bool showHelp;
	bool showSettings;

};

#endif