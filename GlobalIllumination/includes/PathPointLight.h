#ifndef PATH_POINT_LIGHT_H
#define PATH_POINT_LIGHT_H

#include <PointLight.h>

// PathPointLight
//
// Point light that follows a simple rectangular path on the XZ-plane.
class PathPointLight
{
public:
	PathPointLight() :
		pointLight(NULL),
		paused(false)
	{
	}

	bool Init(const Vector3 &position, float radius, const Color &color, float multiplier, const Vector3 &direction);

	void Update();

	void SetActive(bool active)
	{
		pointLight->SetActive(active);
	}

	void SetPaused(bool paused)
	{
		this->paused = paused;
	}

	static void SetControlPoints(float minX, float maxX, float minZ, float maxZ)
	{
		controlPoints[0] = minX;
		controlPoints[1] = maxX;
		controlPoints[2] = minZ;
		controlPoints[3] = maxZ;
	}

private:
	PointLight *pointLight;
	Vector3 direction;
	bool paused;
	static float controlPoints[4];

};

#endif