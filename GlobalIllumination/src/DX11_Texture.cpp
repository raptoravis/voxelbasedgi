#include <stdafx.h>
#include <Demo.h>
#include <DX11_Texture.h>

void DX11_Texture::Release()
{
	SAFE_RELEASE(texture);
	SAFE_RELEASE(shaderResourceView);
	SAFE_RELEASE(unorderedAccessView);
}

bool DX11_Texture::LoadFromFile(const char *fileName)
{
	strcpy(name, fileName);
	char filePath[DEMO_MAX_FILEPATH];
	if (!Demo::fileManager->GetFilePath(fileName, filePath))
		return false;

	if (D3DX11CreateTextureFromFile(Demo::renderer->GetDevice(), filePath, NULL, NULL, &texture, NULL) != S_OK)
		return false;

	if (Demo::renderer->GetDevice()->CreateShaderResourceView(texture, NULL, &shaderResourceView) != S_OK)
		return false;

	return true;
}

bool DX11_Texture::CreateRenderable(unsigned int width, unsigned int height, unsigned int depth, texFormats format,
	unsigned int numLevels, unsigned int rtFlags)
{
	strcpy(name, "renderTargetTexture");
	const bool useUAV = rtFlags & UAV_RTF;
	this->numLevels = numLevels;
	bool typeless = (format == TEX_FORMAT_DEPTH24) || (rtFlags & SRGB_RTF);
	DXGI_FORMAT textureFormat = GetDX11TexFormat(format, typeless);
	if (textureFormat == DXGI_FORMAT_UNKNOWN)
		return false;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.ArraySize = depth;
	textureDesc.MipLevels = numLevels;
	textureDesc.Format = textureFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	if (format != TEX_FORMAT_DEPTH24)
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	else
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	if (useUAV)
		textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = (numLevels == 1) ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS;
	if (Demo::renderer->GetDevice()->CreateTexture2D(&textureDesc, NULL, (ID3D11Texture2D**)&texture) != S_OK)
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = GetDX11TexFormat(format);
	if (depth == 1)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = numLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = numLevels;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.ArraySize = depth;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
	}

	if (Demo::renderer->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &shaderResourceView) != S_OK)
		return false;

	if (useUAV)
	{
		if (Demo::renderer->GetDevice()->CreateUnorderedAccessView(texture, NULL, &unorderedAccessView) != S_OK)
			return false;
	}

	return true;
}

void DX11_Texture::Bind(textureBP bindingPoint, shaderTypes shaderType) const
{
	switch (shaderType)
	{
	case VERTEX_SHADER:
		Demo::renderer->GetDeviceContext()->VSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case GEOMETRY_SHADER:
		Demo::renderer->GetDeviceContext()->GSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case FRAGMENT_SHADER:
		Demo::renderer->GetDeviceContext()->PSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case COMPUTE_SHADER:
		Demo::renderer->GetDeviceContext()->CSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;
	}
}

bool DX11_Texture::IsSrgbFormat(texFormats texFormat)
{
	return((texFormat == TEX_FORMAT_BGR8_SRGB) || (texFormat == TEX_FORMAT_BGRA8_SRGB) || (texFormat == TEX_FORMAT_RGBA8_SRGB) ||
		(texFormat == TEX_FORMAT_DXT1_SRGB) || (texFormat == TEX_FORMAT_DXT3_SRGB) || (texFormat == TEX_FORMAT_DXT5_SRGB));
}

texFormats DX11_Texture::ConvertToSrgbFormat(texFormats texFormat)
{
	if (IsSrgbFormat(texFormat))
	{
		return texFormat;
	}
	else if ((texFormat == TEX_FORMAT_BGR8) || (texFormat == TEX_FORMAT_BGRA8) || (texFormat == TEX_FORMAT_RGBA8) ||
		(texFormat == TEX_FORMAT_DXT1) || (texFormat == TEX_FORMAT_DXT3) || (texFormat == TEX_FORMAT_DXT5))
	{
		return ((texFormats)(texFormat + 1));
	}
	else
	{
		return TEX_FORMAT_NONE;
	}
}

texFormats DX11_Texture::ConvertFromSrgbFormat(texFormats texFormat)
{
	if ((texFormat == TEX_FORMAT_BGR8) || (texFormat == TEX_FORMAT_BGRA8) || (texFormat == TEX_FORMAT_RGBA8) ||
		(texFormat == TEX_FORMAT_DXT1) || (texFormat == TEX_FORMAT_DXT3) || (texFormat == TEX_FORMAT_DXT5))
	{
		return texFormat;
	}
	else if (IsSrgbFormat(texFormat))
	{
		return ((texFormats)(texFormat - 1));
	}
	else
	{
		return TEX_FORMAT_NONE;
	}
}

DXGI_FORMAT DX11_Texture::GetDX11TexFormat(texFormats texFormat, bool typeless)
{
	if (!typeless)
		return (DXGI_FORMAT)texFormat;

	switch (texFormat)
	{
	case TEX_FORMAT_DEPTH24:
		return DXGI_FORMAT_R24G8_TYPELESS;

	case TEX_FORMAT_BGR8:
	case TEX_FORMAT_BGR8_SRGB:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;

	case TEX_FORMAT_BGRA8:
	case TEX_FORMAT_BGRA8_SRGB:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;

	case TEX_FORMAT_RGBA8:
	case TEX_FORMAT_RGBA8_SRGB:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case TEX_FORMAT_DXT1:
	case TEX_FORMAT_DXT1_SRGB:
		return DXGI_FORMAT_BC1_TYPELESS;

	case TEX_FORMAT_DXT3:
	case TEX_FORMAT_DXT3_SRGB:
		return DXGI_FORMAT_BC2_TYPELESS;

	case TEX_FORMAT_DXT5:
	case TEX_FORMAT_DXT5_SRGB:
		return DXGI_FORMAT_BC3_TYPELESS;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}




