#include <stdafx.h>
#include <Demo.h>
#include <Window.h>

// windows procedure
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // send event message to AntTweakBar
  if(TwEventWin(hWnd, uMsg, wParam, lParam))
    return 0; 

	switch(uMsg)
	{
	// prevent activation of screen saver/ turning monitor off
	case WM_SYSCOMMAND:
		{
			switch(wParam)
			{
			case SC_SCREENSAVE: 
			case SC_MONITORPOWER: 
				return 0;
			}
			break;
		}

	// post quit message, when user tries to close window
	case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	if(Demo::inputManager->GetInputMessages(uMsg, wParam))
		return 0;

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool Window::Create()
{
  // get handle to the module instance 
  hInstance = (HINSTANCE)GetModuleHandle(NULL);

  // register window class
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX); 
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance =	hInstance;
	wc.lpszClassName = "Demo"; 
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Failed to register window class!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// create window
	DWORD exStyle = WS_EX_APPWINDOW;
	DWORD style = WS_CAPTION | WS_VISIBLE;
	RECT windowRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}; 
	AdjustWindowRectEx(&windowRect, style, false, exStyle);
	if(!(hWnd = CreateWindowEx(exStyle, "Demo", "Rasterized Voxel-based Dynamic Global Illumination", style,
		                         10, 10, windowRect.right-windowRect.left, windowRect.bottom-windowRect.top,
		                         NULL, NULL, hInstance, NULL)))
	{
		MessageBox(NULL, "Failed to create window!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		Destroy();
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	
	return true;
}

void Window::Destroy()
{
	// show cursor
	ShowCursor(true);

	// destroy window
	if(hWnd)
	{
		if(!DestroyWindow(hWnd))
			MessageBox(NULL, "Could not destroy window!", "ERROR", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;
	}
  
	// unregister window class
	UnregisterClass("Demo", hInstance);
	hInstance = NULL;
}

bool Window::HandleMessages() const
{
	MSG msg;	
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
	{
		if(msg.message == WM_QUIT)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

