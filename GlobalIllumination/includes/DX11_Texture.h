#ifndef DX11_TEXTURE_H
#define DX11_TEXTURE_H

#include <render_states.h>
#include <DX11_Sampler.h>

// DX11_Texture
//
// Manages a texture.
class DX11_Texture
{
public:
  friend class DX11_Renderer;
	friend class DX11_RenderTarget; 

	DX11_Texture():
    numLevels(1),
    texture(NULL),     
    shaderResourceView(NULL),
    unorderedAccessView(NULL)
	{
	  name[0] = 0;
	}

	~DX11_Texture()
	{
		Release();
	}

	void Release();	

	bool LoadFromFile(const char *fileName); 

	// creates render-target texture
	bool CreateRenderable(unsigned int width, unsigned int height, unsigned int depth, texFormats format, 
                        unsigned int numLevels=1, unsigned int rtFlags=0);	

	void Bind(textureBP bindingPoint, shaderTypes shaderType=VERTEX_SHADER) const;

	const char* GetName() const
	{
		return name;
	}

  unsigned int GetNumLevels() const
  {
    return numLevels;
  }

  ID3D11UnorderedAccessView* GetUnorderdAccessView() const
  {
    return unorderedAccessView;
  }

  static bool IsSrgbFormat(texFormats texFormat);

  static texFormats ConvertToSrgbFormat(texFormats texFormat);

  static texFormats ConvertFromSrgbFormat(texFormats texFormat);

  static DXGI_FORMAT GetDX11TexFormat(texFormats texFormat, bool typeless=false);

private:	  
	char name[DEMO_MAX_FILENAME];
  unsigned int numLevels;

	ID3D11Resource *texture;
	ID3D11ShaderResourceView *shaderResourceView;
	ID3D11UnorderedAccessView *unorderedAccessView;

};

#endif
