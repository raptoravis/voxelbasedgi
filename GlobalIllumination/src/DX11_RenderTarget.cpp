#include <stdafx.h>
#include <Demo.h>
#include <DX11_StructuredBuffer.h>
#include <DX11_RenderTarget.h>

void DX11_RenderTarget::Release()
{
	SAFE_DELETE_ARRAY(frameBufferTextures);
	if (renderTargetViews)
	{
		for (unsigned int i = 0; i < numColorBuffers; i++)
			SAFE_RELEASE(renderTargetViews[i]);
		SAFE_DELETE_ARRAY(renderTargetViews);
	}
	SAFE_DELETE(depthStencilTexture);
	for (unsigned int i = 0; i < 2; i++)
		SAFE_RELEASE(depthStencilViews[i]);
}

bool DX11_RenderTarget::Create(const RenderTargetDesc &desc)
{
	width = desc.width;
	height = desc.height;
	depth = desc.depth;
	numColorBuffers = desc.CalcNumColorBuffers();

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	if (numColorBuffers > 0)
	{
		clearMask = COLOR_CLEAR_BIT;
		frameBufferTextures = new DX11_Texture[numColorBuffers];
		if (!frameBufferTextures)
			return false;
		renderTargetViews = new ID3D11RenderTargetView*[numColorBuffers];
		if (!renderTargetViews)
			return false;
		for (unsigned int i = 0; i < numColorBuffers; i++)
		{
			unsigned int flags = desc.colorBufferDescs[i].rtFlags;
			if (DX11_Texture::IsSrgbFormat(desc.colorBufferDescs[i].format))
				flags |= SRGB_RTF;
			texFormats rtvFormat;
			if (flags & SRGB_RTF)
			{
				if (rtFlags & SRGB_RTF)
					rtvFormat = DX11_Texture::ConvertToSrgbFormat(desc.colorBufferDescs[i].format);
				else
					rtvFormat = DX11_Texture::ConvertFromSrgbFormat(desc.colorBufferDescs[i].format);
			}
			else
			{
				rtvFormat = desc.colorBufferDescs[i].format;
			}
			if (rtvFormat == TEX_FORMAT_NONE)
				return false;

			if (!frameBufferTextures[i].CreateRenderable(width, height, depth, desc.colorBufferDescs[i].format, desc.numLevels, flags))
			{
				return false;
			}

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = DX11_Texture::GetDX11TexFormat(rtvFormat);
			if (depth == 1)
			{
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;
			}
			else
			{
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Texture2DArray.MipSlice = 0;
				rtvDesc.Texture2DArray.ArraySize = depth;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
			}
			if (Demo::renderer->GetDevice()->CreateRenderTargetView(frameBufferTextures[i].texture, &rtvDesc, &renderTargetViews[i]) != S_OK)
				return false;
		}
	}

	if (desc.depthStencilBufferDesc.format != TEX_FORMAT_NONE)
	{
		clearMask |= DEPTH_CLEAR_BIT | STENCIL_CLEAR_BIT;
		depthStencilTexture = new DX11_Texture;
		if (!depthStencilTexture)
			return false;
		if (!depthStencilTexture->CreateRenderable(width, height, depth, desc.depthStencilBufferDesc.format, desc.numLevels,
			desc.depthStencilBufferDesc.rtFlags))
		{
			return false;
		}
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		if (Demo::renderer->GetDevice()->CreateDepthStencilView(depthStencilTexture->texture, &descDSV, &depthStencilViews[0]) != S_OK)
			return false;
		descDSV.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
		if (Demo::renderer->GetDevice()->CreateDepthStencilView(depthStencilTexture->texture, &descDSV, &depthStencilViews[1]) != S_OK)
			return false;
	}

	return true;
}

bool DX11_RenderTarget::CreateBackBuffer()
{
	width = SCREEN_WIDTH;
	height = SCREEN_HEIGHT;
	depth = 1;
	clearMask = COLOR_CLEAR_BIT | DEPTH_CLEAR_BIT | STENCIL_CLEAR_BIT;
	numColorBuffers = 1;

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	ID3D11Texture2D* backBufferTexture = NULL;
	if (Demo::renderer->GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture) != S_OK)
		return false;
	renderTargetViews = new ID3D11RenderTargetView*[numColorBuffers];
	if (!renderTargetViews)
		return false;
	if (Demo::renderer->GetDevice()->CreateRenderTargetView(backBufferTexture, NULL, &renderTargetViews[0]) != S_OK)
	{
		backBufferTexture->Release();
		return false;
	}
	backBufferTexture->Release();

	depthStencilTexture = new DX11_Texture;
	if (!depthStencilTexture)
		return false;
	if (!depthStencilTexture->CreateRenderable(width, height, depth, TEX_FORMAT_DEPTH24))
		return false;
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	if (Demo::renderer->GetDevice()->CreateDepthStencilView(depthStencilTexture->texture, &descDSV, &depthStencilViews[0]) != S_OK)
		return false;
	descDSV.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
	if (Demo::renderer->GetDevice()->CreateDepthStencilView(depthStencilTexture->texture, &descDSV, &depthStencilViews[1]) != S_OK)
		return false;

	return true;
}

void DX11_RenderTarget::Bind(const DX11_RenderTargetConfig *rtConfig)
{
	if (renderTargetViews || depthStencilViews[0])
		Demo::renderer->GetDeviceContext()->RSSetViewports(1, &viewport);

	if (!rtConfig)
	{
		Demo::renderer->GetDeviceContext()->OMSetRenderTargets(numColorBuffers, renderTargetViews, depthStencilViews[0]);
	}
	else
	{
		const RtConfigDesc &rtConfigDesc = rtConfig->GetDesc();
		ID3D11DepthStencilView *setDSV = (rtConfigDesc.flags & DS_READ_ONLY_RTCF) ? depthStencilViews[1] : depthStencilViews[0];
		if (!(rtConfigDesc.flags & COMPUTE_RTCF))
		{
			if (rtConfigDesc.numUnorderedAccessViews == 0)
			{
				Demo::renderer->GetDeviceContext()->OMSetRenderTargets(rtConfigDesc.numColorBuffers,
					&renderTargetViews[rtConfigDesc.firstColorBufferIndex], setDSV);
			}
			else
			{
				assert(rtConfigDesc.numUnorderedAccessViews <= NUM_UAV_VIEW);
				Demo::renderer->GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(numColorBuffers,
					&renderTargetViews[rtConfigDesc.firstColorBufferIndex], setDSV, rtConfigDesc.numColorBuffers,
					rtConfigDesc.numUnorderedAccessViews, rtConfigDesc.unorderedAccessViews, NULL);
			}
		}
		else
		{
			Demo::renderer->GetDeviceContext()->OMSetRenderTargets(0, NULL, NULL);
			if (rtConfigDesc.numUnorderedAccessViews == 0)
			{
				assert(rtConfigDesc.numColorBuffers <= MAX_NUM_COLOR_BUFFERS);
				ID3D11UnorderedAccessView *sbUnorderedAccessViews[MAX_NUM_COLOR_BUFFERS];
				for (unsigned int i = 0; i < rtConfigDesc.numColorBuffers; i++)
					sbUnorderedAccessViews[i] = frameBufferTextures[i].GetUnorderdAccessView();
				Demo::renderer->GetDeviceContext()->CSSetUnorderedAccessViews(0, rtConfigDesc.numColorBuffers,
					&sbUnorderedAccessViews[rtConfigDesc.firstColorBufferIndex], NULL);
			}
			else
			{
				assert(rtConfigDesc.numUnorderedAccessViews <= NUM_UAV_VIEW);
				Demo::renderer->GetDeviceContext()->CSSetUnorderedAccessViews(0, rtConfigDesc.numUnorderedAccessViews, rtConfigDesc.unorderedAccessViews, NULL);
			}
		}
	}

	if (clearTarget)
	{
		Clear(clearMask);
		clearTarget = false;
	}
}

void DX11_RenderTarget::Clear(unsigned int newClearMask) const
{
	if (renderTargetViews)
	{
		if (newClearMask & COLOR_CLEAR_BIT)
		{
			for (unsigned int i = 0; i < numColorBuffers; i++)
				Demo::renderer->GetDeviceContext()->ClearRenderTargetView(renderTargetViews[i], CLEAR_COLOR);
		}
	}
	if (depthStencilViews[0])
	{
		newClearMask &= ~COLOR_CLEAR_BIT;
		if (newClearMask != 0)
			Demo::renderer->GetDeviceContext()->ClearDepthStencilView(depthStencilViews[0], newClearMask, CLEAR_DEPTH, CLEAR_STENCIL);
	}
}




