#include <stdafx.h>
#include <Demo.h>
#include <DeferredLighting.h>
#include <GlobalIllum.h>
#include <Sky.h>
#include <FinalProcessor.h>
#include <Application.h>

#define INIT_CAMERA_POSITION Vector3(632.0f, 150.0f, -142.0f)
#define INIT_CAMERA_ROTATION Vector3(158.0f, 0.0f, 0.0f)
#define INIT_DYN_MESH_POSITION Vector3(-100.0f, 0.0f, 28.0f)
#define INIT_DIR_LIGHT_DIRECTION Vector3(0.2403f, -0.9268f, 0.2886f)
#define MOUSE_SPEED 0.5f
#define CAMERA_MOVE_SPEED 0.2f
#define MESH_MOVE_SPEED 0.2f 

// callbacks for AntTweakbar
static void TW_CALL SetGlobalIllumMode(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->SetGlobalIllumMode(*((globalIllumModes*)value));
}

static void TW_CALL GetGlobalIllumMode(void *value, void *clientData)
{
  *((globalIllumModes*)value) = ((GlobalIllum*)clientData)->GetGlobalIllumMode();
}

static void TW_CALL SetLightDirection(const void *value, void *clientData)
{
  ((DirectionalLight*)clientData)->SetDirection(*((Vector3*)value));
}

static void TW_CALL GetLightDirection(void *value, void *clientData)
{
  *((Vector3*)value) = ((DirectionalLight*)clientData)->GetDirection();
}

static void TW_CALL EnablePointLights(const void *value, void *clientData)
{
  ((Application*)clientData)->EnablePathLights(*((const bool*)value));
}

static void TW_CALL ArePointLightsEnabled(void *value, void *clientData)
{
  *((bool*)value) = ((Application*)clientData)->ArePathLightsEnabled();
}

static void TW_CALL AnimatePointLights(const void *value, void *clientData)
{
  ((Application*)clientData)->AnimatePathLights(*((const bool*)value));
}

static void TW_CALL ArePointLightsAnimated(void *value, void *clientData)
{
  *((bool*)value) = ((Application*)clientData)->ArePathLightsAnimated();
}

static void TW_CALL SetFluxAmplifier(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->SetFluxAmplifier(*((const float*)value));
}

static void TW_CALL GetFluxAmplifier(void *value, void *clientData)
{
  *((float*)value) = ((GlobalIllum*)clientData)->GetFluxAmplifier();
}

static void TW_CALL SetOcclusionAmplifier(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->SetOcclusionAmplifier(*((const float*)value));
}

static void TW_CALL GetOcclusionAmplifier(void *value, void *clientData)
{
  *((float*)value) = ((GlobalIllum*)clientData)->GetOcclusionAmplifier();
}

static void TW_CALL SetGlobalIllumPower(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->SetGlobalIllumPower(*((const float*)value));
}

static void TW_CALL GetGlobalIllumPower(void *value, void *clientData)
{
  *((float*)value) = ((GlobalIllum*)clientData)->GetGlobalIllumPower();
}

static void TW_CALL SetNumPropagationSteps(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->SetNumPropagationSteps(*((const unsigned int*)value));
}

static void TW_CALL GetNumPropagationSteps(void *value, void *clientData)
{
  *((unsigned int*)value) = ((GlobalIllum*)clientData)->GetNumPropagationSteps();
}

static void TW_CALL EnableOcclusion(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->EnableOcclusion(*((const bool*)value));
}

static void TW_CALL IsOcclusionEnabled(void *value, void *clientData)
{
  *((bool*)value) = ((GlobalIllum*)clientData)->IsOcclusionEnabled();
}

static void TW_CALL EnableSmoothing(const void *value, void *clientData)
{
  ((GlobalIllum*)clientData)->EnableSmoothing(*((const bool*)value));
}

static void TW_CALL IsSmoothingEnabled(void *value, void *clientData)
{
  *((bool*)value) = ((GlobalIllum*)clientData)->IsSmoothingEnabled();
}


bool Application::Init()
{
	if(!Demo::Create())
		return false;

	char exeDirectory[DEMO_MAX_FILEPATH];
	if((!Demo::fileManager->GetExeDirectory(exeDirectory)) ||
		(!Demo::fileManager->SetWorkDirectory(exeDirectory)))
  {
		return false;
  }
	Demo::fileManager->AddDirectory("../Data/");

	if(!Demo::window->Create())
		return false;

	if(!Demo::renderer->Create())
		return false;

	Demo::timeManager->Init();

  // init AntTweakBar
  if(!TwInit(TW_DIRECT3D11, Demo::renderer->GetDevice()))
    return false;

	if(!Demo::renderer->CreatePostProcessor<DeferredLighting>())
		return false;

	// GlobalIllum post-processor is responsible for generating dynamic global illumination
	globalIllum = Demo::renderer->CreatePostProcessor<GlobalIllum>();
	if(!globalIllum)
		return false;

	if(!Demo::renderer->CreatePostProcessor<Sky>())
		return false;

	if(!Demo::renderer->CreatePostProcessor<FinalProcessor>())
		return false;

	if(!OnInit())
		return false;

	return true;
}

void Application::Run()
{
	// main loop of application
	while(!quit)
	{
		// handle window messages
		if(!Demo::window->HandleMessages())
			quit = true;

		Demo::timeManager->Update();

		Demo::renderer->ClearFrame();

		OnRun();

		Demo::renderer->UpdateLights();

		const unsigned int numDemoMeshes = Demo::resourceManager->GetNumDemoMeshes();
		for(unsigned int i=0; i<numDemoMeshes; i++) 
			Demo::resourceManager->GetDemoMesh(i)->AddSurfaces();

		const unsigned int numFonts = Demo::resourceManager->GetNumFonts();
		for(unsigned int i=0; i<numFonts; i++) 
			Demo::resourceManager->GetFont(i)->AddSurfaces();

    // execute all GPU commands
    Demo::renderer->ExecuteGpuCmds();

		Demo::inputManager->Update();
	}
}

void Application::Shutdown()
{
  // terminate AntTweakBar
  TwTerminate();

	Demo::Release();
}

void Application::Quit()
{
	quit = true;
}

bool Application::OnInit()
{
	// cache pointer to main camera
	mainCamera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	if(!mainCamera)
		return false;

	// set initial camera position/ rotation 
	mainCamera->Update(INIT_CAMERA_POSITION, INIT_CAMERA_ROTATION);

  // init previous mouse position
  prevMousePos = Demo::inputManager->GetMousePos();

	// create default font for displaying info
	defaultFont = Demo::resourceManager->LoadFont("fonts/arial.font");
	if(!defaultFont)
		return false;
	
	// load sponza mesh
	if(!Demo::resourceManager->LoadDemoMesh("meshes/sponza.mesh"))
		return false;

	// load dwarf mesh
	dynamicMesh = Demo::resourceManager->LoadDemoMesh("meshes/dwarf.mesh");
	if(!dynamicMesh)
		return false;
	dynamicMesh->SetPosition(INIT_DYN_MESH_POSITION);

	// create directional light
	dirLight = Demo::renderer->CreateDirectionalLight(INIT_DIR_LIGHT_DIRECTION, Color(1.0f, 1.0f, 1.0f), 2.5f);
	if(!dirLight)
		return false;

	// set control-points of path for all moving point-lights  
	PathPointLight::SetControlPoints(-1350.0f, 1250.0f, -600.0f, 500.0f);

  // create point lights that follow a simple path
	if(!pathPointLights[0].Init(Vector3(-550.0f,10.0f,500.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(1.0f, 0.0f, 0.0f)))
		return false;
  if(!pathPointLights[1].Init(Vector3(550.0f, 10.0f, 500.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[2].Init(Vector3(-550.0f, 10.0f, -600.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(-1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[3].Init(Vector3(550.0f, 10.0f, -600.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(-1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[4].Init(Vector3(-1350.0f, 10.0f, -30.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(0.0f, 0.0f, 1.0f)))
		return false;
	if(!pathPointLights[5].Init(Vector3(1250.0f, 10.0f, -30.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(0.0f, 0.0f, -1.0f)))
		return false;
	if(!pathPointLights[6].Init(Vector3(1250.0f, 720.0f, 450.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[7].Init(Vector3(1200.0f, 720.0f, -600.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[8].Init(Vector3(-1350.0f, 720.0f, 460.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(-1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[9].Init(Vector3(-1320.0f, 720.0f, -600.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(-1.0f, 0.0f, 0.0f)))
		return false;
	if(!pathPointLights[10].Init(Vector3(-40.0f, 720.0f, 500.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(0.0f, 0.0f, 1.0f)))
		return false;
	if(!pathPointLights[11].Init(Vector3(-40.0f, 720.0f, -600.0f), 260.0f, Color(1.0f, 1.0f, 0.8f), 2.5f, Vector3(0.0f, 0.0f, -1.0f)))
		return false;

  // create a tweak bar 
  TwBar *tweakBar = TwNewBar("Settings");
  if(!tweakBar)
    return false;

  // configure tweak bars
  TwDefine("GLOBAL help='WASD - Move camera\nRMB - Enable mouse look\nIKJLUO - Move dynamic mesh\nESC - Quit application\nR - Reset\nP - Screenshot'"); 
  TwDefine("GLOBAL fontsize=3");  
  TwDefine("GLOBAL fontresizable=false"); 
  TwDefine("Settings visible=false"); 
  TwDefine("Settings iconified=false"); 
  TwDefine("Settings iconifiable=false"); 
  TwDefine("TW_HELP position='30 120'");
  TwDefine("TW_HELP size='535 325'");
  TwDefine("TW_HELP visible=false"); 
  TwDefine("TW_HELP iconified=false"); 
  TwDefine("TW_HELP iconifiable=false"); 
  const int barSize[2] = {400, 400};
  TwSetParam(tweakBar, NULL, "size", TW_PARAM_INT32, 2, barSize);
  const int barPosition[2] = { SCREEN_WIDTH - 410, 10 };
  TwSetParam(tweakBar, nullptr, "position", TW_PARAM_INT32, 2, barPosition);
  const int valueWidth = 200;
  TwSetParam(tweakBar, nullptr, "valueswidth", TW_PARAM_INT32, 1, &valueWidth);

  // add variables to tweak bar 
  const unsigned int numModes = 4;
  TwEnumVal modeEnumValues[numModes] =
  {
    {DIRECT_ILLUM_ONLY_GIM, "Direct"},
    {INDIRECT_ILLUM_ONLY_GIM, "Indirect"}, 
    {DEFAULT_GIM, "Direct+Indirect"},
    {VISUALIZE_GRIDS_GIM, "Voxel Grids"}
  }; 
  TwType modeType = TwDefineEnum("ModeType", modeEnumValues, numModes);
  const char *modeDefinition =
  {
    "help='Direct = Direct Illumination\nIndirect = Indirect Illumination\nVoxel Grids = Visualize voxel grids'"
  };
  TwAddVarCB(tweakBar, "Mode", modeType, SetGlobalIllumMode, GetGlobalIllumMode, globalIllum, modeDefinition);
  TwAddVarCB(tweakBar, "Flux amplifier", TW_TYPE_FLOAT, SetFluxAmplifier, GetFluxAmplifier, globalIllum, "step=0.01 group=GlobalIllum");
  TwAddVarCB(tweakBar, "Occlusion amplifier", TW_TYPE_FLOAT, SetOcclusionAmplifier, GetOcclusionAmplifier, globalIllum, "step=0.01 group=GlobalIllum");
  TwAddVarCB(tweakBar, "GI-contribution power", TW_TYPE_FLOAT, SetGlobalIllumPower, GetGlobalIllumPower, globalIllum, "step=0.01 group=GlobalIllum");
  TwAddVarCB(tweakBar, "Propagation steps", TW_TYPE_UINT32, SetNumPropagationSteps, GetNumPropagationSteps, globalIllum, "group=GlobalIllum");
  TwAddVarCB(tweakBar, "Occlusion", TW_TYPE_BOOLCPP, EnableOcclusion, IsOcclusionEnabled, globalIllum, "group=GlobalIllum");
  TwAddVarCB(tweakBar, "Smoothing", TW_TYPE_BOOLCPP, EnableSmoothing, IsSmoothingEnabled, globalIllum, "group=GlobalIllum");
  TwAddVarCB(tweakBar, "Direction", TW_TYPE_DIR3F, SetLightDirection, GetLightDirection, dirLight, "group=DirectionalLight opened=true showval=true");
  TwAddVarCB(tweakBar, "Enable", TW_TYPE_BOOLCPP, EnablePointLights, ArePointLightsEnabled, this, "group=PointLights");
  TwAddVarCB(tweakBar, "Animate", TW_TYPE_BOOLCPP, AnimatePointLights, ArePointLightsAnimated, this, "group=PointLights");
  
	return true;
}

void Application::HandleInput()
{ 
  // update camera
  Vector3 cameraRotation = mainCamera->GetRotation();
  POINT currentMousePos = Demo::inputManager->GetMousePos();
  if(Demo::inputManager->GetTriggerState(VK_RBUTTON))
  { 
    Demo::inputManager->ShowMouseCursor(false);
    cameraRotation.x -= (currentMousePos.x-prevMousePos.x)*MOUSE_SPEED;
    cameraRotation.y += (currentMousePos.y-prevMousePos.y)*MOUSE_SPEED;
    CLAMP(cameraRotation.y, -80.0f, 80.0f);
    Demo::inputManager->SetMousePos(prevMousePos);
  }
  else
  {
    Demo::inputManager->ShowMouseCursor(true);
    prevMousePos = currentMousePos;
  }
	
	Vector3 velocity;
	if(Demo::inputManager->GetTriggerState(char('W'))) // move forward
	{
		velocity.x = -sin(DEG2RAD(cameraRotation.x));
		velocity.y = -tan(DEG2RAD(cameraRotation.y));
		velocity.z = -cos(DEG2RAD(cameraRotation.x));
	}
	if(Demo::inputManager->GetTriggerState(char('S'))) // move backward
	{
		velocity.x = sin(DEG2RAD(cameraRotation.x));
		velocity.y = tan(DEG2RAD(cameraRotation.y));
		velocity.z = cos(DEG2RAD(cameraRotation.x));
	}
	if(Demo::inputManager->GetTriggerState(char('A'))) // move left
	{
		velocity.x = sin(DEG2RAD(cameraRotation.x-90.0f));
		velocity.z = cos(DEG2RAD(cameraRotation.x-90.0f));
	}
	if(Demo::inputManager->GetTriggerState(char('D'))) // move right
	{
		velocity.x = -sin(DEG2RAD(cameraRotation.x-90.0f));
		velocity.z = -cos(DEG2RAD(cameraRotation.x-90.0f));
	}
	Vector3 cameraPosition = mainCamera->GetPosition();
	cameraPosition += velocity.GetNormalized()*CAMERA_MOVE_SPEED*(float)Demo::timeManager->GetFrameInterval();

	mainCamera->Update(cameraPosition, cameraRotation);

  // move dynamic mesh
  Vector3 meshPosition = dynamicMesh->GetPosition();
	if(Demo::inputManager->GetTriggerState(char('I')))
		meshPosition.x += (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	if(Demo::inputManager->GetTriggerState(char('K')))
		meshPosition.x -= (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	if(Demo::inputManager->GetTriggerState(char('J')))
		meshPosition.z += (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	if(Demo::inputManager->GetTriggerState(char('L')))
		meshPosition.z -= (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	if(Demo::inputManager->GetTriggerState(char('U')))
		meshPosition.y += (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	if(Demo::inputManager->GetTriggerState(char('O')))
		meshPosition.y -= (float)Demo::timeManager->GetFrameInterval()*MESH_MOVE_SPEED;
	dynamicMesh->SetPosition(meshPosition);

	// toggle help 
	if(Demo::inputManager->GetSingleTriggerState(VK_F1))
  {
		showHelp = !showHelp;
    if(showHelp)
      TwDefine("TW_HELP visible=true");  
    else
      TwDefine("TW_HELP visible=false"); 
  }

  // toggle settings
  if(Demo::inputManager->GetSingleTriggerState(VK_F2))
  {
    showSettings = !showSettings;
    if(showSettings)
      TwDefine("Settings visible=true");  
    else
      TwDefine("Settings visible=false"); 
  }

	// reset settings
  if(Demo::inputManager->GetSingleTriggerState(char('R')))
	{
		mainCamera->Update(INIT_CAMERA_POSITION, INIT_CAMERA_ROTATION);
		dirLight->SetDirection(INIT_DIR_LIGHT_DIRECTION);
    dynamicMesh->SetPosition(INIT_DYN_MESH_POSITION);
		globalIllum->Reset();
		pathLightsAnimated = true;
		for(unsigned int i=0; i<NUM_PATH_POINT_LIGHTS; i++)
    {
		  pathPointLights[i].SetPaused(false);
      pathPointLights[i].SetActive(true);
    }
		pathLigthsEnabled = true;
	}

	// save screen-shot 
	if(Demo::inputManager->GetSingleTriggerState(char('P')))
    Demo::renderer->SaveScreenshot();
 
  // quit application by pressing ESCAPE
  if(Demo::inputManager->GetSingleTriggerState(VK_ESCAPE))
    Quit();
}

void Application::DisplayInfo()
{
  defaultFont->Print(Vector2(-0.95f, 0.86f), 0.04f, Color(0.0f, 1.0f, 0.0f), "FPS: %.2f", Demo::timeManager->GetFPS());
  
  if(showHelp)
    defaultFont->Print(Vector2(-0.95f, 0.78f), 0.03f, Color(1.0f, 1.0f, 0.0f), "F1 - Hide help");
  else
    defaultFont->Print(Vector2(-0.95f, 0.78f), 0.03f, Color(1.0f, 1.0f, 0.0f), "F1 - Show help");

  if(showSettings)
    defaultFont->Print(Vector2(-0.95f, 0.72f), 0.03f, Color(1.0f, 1.0f, 0.0f), "F2 - Hide settings");
  else
    defaultFont->Print(Vector2(-0.95f, 0.72f), 0.03f, Color(1.0f, 1.0f, 0.0f), "F2 - Show settings");
}

void Application::OnRun()
{		
  HandleInput();	

  DisplayInfo();
	
	if(pathLigthsEnabled && pathLightsAnimated)
	{
		for(unsigned int i=0; i<NUM_PATH_POINT_LIGHTS; i++)
			pathPointLights[i].Update();
	}
}

void Application::EnablePathLights(bool enable)
{
  pathLigthsEnabled = enable;
  for(unsigned int i=0; i<NUM_PATH_POINT_LIGHTS; i++)
    pathPointLights[i].SetActive(pathLigthsEnabled);
}

void Application::AnimatePathLights(bool animate)
{
  pathLightsAnimated = animate;
  for(unsigned int i=0; i<NUM_PATH_POINT_LIGHTS; i++)
    pathPointLights[i].SetPaused(!pathLightsAnimated);
}
