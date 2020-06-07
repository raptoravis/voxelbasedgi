#include <stdafx.h>
#include <Demo.h>
#include <PathPointLight.h>

#define PATH_POINTLIGHT_MOVE_SPEED 0.1f

float PathPointLight::controlPoints[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

bool PathPointLight::Init(const Vector3 &position, float radius, const Color &color, float multiplier, const Vector3 &direction)
{
	pointLight = Demo::renderer->CreatePointLight(position, radius, color, multiplier);
	if (!pointLight)
		return false;
	this->direction = direction;
	return true;
}

void PathPointLight::Update()
{
	if ((!pointLight->IsActive()) || (paused))
		return;

	Vector3 position = pointLight->GetPosition();
	if (position.x > controlPoints[1])
	{
		position.x = controlPoints[1];
		direction.Set(0.0f, 0.0f, -1.0f);
	}
	if (position.x < controlPoints[0])
	{
		position.x = controlPoints[0];
		direction.Set(0.0f, 0.0f, 1.0f);
	}
	if (position.z > controlPoints[3])
	{
		position.z = controlPoints[3];
		direction.Set(1.0f, 0.0f, 0.0f);
	}
	if (position.z < controlPoints[2])
	{
		position.z = controlPoints[2];
		direction.Set(-1.0f, 0.0f, 0.0f);
	}

	// prevent large values at beginning of application
	float frameInterval = (float)Demo::timeManager->GetFrameInterval();
	if (frameInterval > 1000.0f)
		return;

	position += direction * frameInterval*PATH_POINTLIGHT_MOVE_SPEED;
	pointLight->SetPosition(position);
}
