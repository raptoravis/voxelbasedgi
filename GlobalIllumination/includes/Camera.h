#ifndef CAMERA_H
#define CAMERA_H

class DX11_UniformBuffer;

// Camera
//
// Simple camera.
class Camera
{
public:
  struct BufferData
  {
    BufferData():
      nearClipDistance(0.0f),
      farClipDistance(0.0f),
      nearFarClipDistance(0.0f)
    {
    }

    Matrix4 viewMatrix;
    Matrix4 projMatrix;
    Matrix4 viewProjMatrix;
    Matrix4 invViewProjMatrix;
    Vector3 position;
    float nearClipDistance;
    float farClipDistance;
    float nearFarClipDistance;
  };

  Camera():
    fovy(0.0f),
    aspectRatio(0.0f),
    uniformBuffer(NULL)
	{
	}

	bool Init(float fovy, float aspectRatio, float nearClipDistance, float farClipDistance);

	void Update(const Vector3 &position, const Vector3 &rotation);

	DX11_UniformBuffer* GetUniformBuffer() const
	{
		return uniformBuffer;
	}

	Matrix4 GetViewMatrix() const
	{
		return bufferData.viewMatrix;
	}

 	Matrix4 GetProjMatrix() const
	{
		return bufferData.projMatrix;
	}

	Matrix4 GetInvProjMatrix() const
	{
		return invProjMatrix;
	}

	Matrix4 GetViewProjMatrix() const
	{
		return bufferData.viewProjMatrix;
	}
 
  Matrix4 GetInvViewProjMatrix() const
  {
    return bufferData.invViewProjMatrix;
  }

	Vector3 GetPosition() const
	{
		return bufferData.position;
	}
	
	Vector3 GetRotation() const
	{
		return rotation;
	}

  Vector3 GetDirection() const
	{
		return direction;
	}

	float GetFovy() const
	{
		return fovy;
	}

	float GetAspectRatio() const
	{
		return aspectRatio;
	}

	float GetNearClipDistance() const
	{
		return bufferData.nearClipDistance;
	}

  float GetFarClipDistance() const
	{
		return bufferData.farClipDistance;
	}

	float GetNearFarClipDistance() const
	{
		return bufferData.nearFarClipDistance;
	}

private:
  void UpdateUniformBuffer();

	// data for camera uniform-buffer
  BufferData bufferData;
	
	Vector3 rotation;
	Vector3 direction;
	float fovy;
	float aspectRatio;
	Matrix4 invProjMatrix;
	DX11_UniformBuffer *uniformBuffer;

};

#endif
