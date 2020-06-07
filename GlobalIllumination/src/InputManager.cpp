#include <stdafx.h>
#include <Demo.h>
#include <InputManager.h>

#define KEY_PRESSED 1       // flag, indicating that key was pressed
#define KEY_QUERIED 2       // flag, indicating that key was queried
#define KEY_MULTI_PRESSED 4 // flag, indicating that key was pressed more than once

InputManager::InputManager() :
	cursorVisible(true)
{
	memset(keyInfos, 0, NUM_KEY_INFOS * sizeof(unsigned int));
}

bool InputManager::GetInputMessages(UINT uMsg, WPARAM wParam)
{
	switch (uMsg)
	{
		// keyboard key is pressed down  
	case WM_KEYDOWN:
	{
		SetTriggerState(wParam, true);
		return true;
	}

	// keyboard key is released
	case WM_KEYUP:
	{
		SetTriggerState(wParam, false);
		return true;
	}

	// left mouse-button is pressed down
	case WM_LBUTTONDOWN:
	{
		SetTriggerState(VK_LBUTTON, true);
		return true;
	}

	// left mouse-button is released
	case WM_LBUTTONUP:
	{
		SetTriggerState(VK_LBUTTON, false);
		return true;
	}

	// right mouse-button is pressed down
	case WM_RBUTTONDOWN:
	{
		SetTriggerState(VK_RBUTTON, true);
		return true;
	}

	// right mouse-button is released
	case WM_RBUTTONUP:
	{
		SetTriggerState(VK_RBUTTON, false);
		return true;
	}
	}

	return false;
}

void InputManager::Update()
{
	for (unsigned int i = 0; i < NUM_KEY_INFOS; i++)
	{
		if (keyInfos[i] & KEY_QUERIED)
			keyInfos[i] |= KEY_MULTI_PRESSED;
	}
}

void InputManager::SetTriggerState(unsigned char keyCode, bool pressed)
{
	if (pressed)
	{
		keyInfos[keyCode] |= KEY_PRESSED;
	}
	else
	{
		keyInfos[keyCode] &= ~KEY_PRESSED;
		keyInfos[keyCode] &= ~KEY_QUERIED;
		keyInfos[keyCode] &= ~KEY_MULTI_PRESSED;
	}
}

bool InputManager::GetTriggerState(unsigned char keyCode)
{
	return (keyInfos[keyCode] & KEY_PRESSED);
}

bool InputManager::GetSingleTriggerState(unsigned char keyCode)
{
	if ((keyInfos[keyCode] & KEY_PRESSED) && (!(keyInfos[keyCode] & KEY_MULTI_PRESSED)))
	{
		keyInfos[keyCode] |= KEY_QUERIED;
		return true;
	}
	return false;
}

POINT InputManager::GetMousePos(bool windowSpace) const
{
	POINT pos;
	GetCursorPos(&pos);
	if (windowSpace)
		MapWindowPoints(NULL, Demo::window->GetHWnd(), &pos, 1);
	return pos;
}

void InputManager::SetMousePos(POINT position, bool windowSpace) const
{
	if (windowSpace)
		MapWindowPoints(Demo::window->GetHWnd(), NULL, &position, 1);
	SetCursorPos(position.x, position.y);
}

void InputManager::ShowMouseCursor(bool show)
{
	if (cursorVisible == show)
		return;

	int counter;
	if (show)
	{
		do
		{
			counter = ShowCursor(true);
		} while (counter < 0);
	}
	else
	{
		do
		{
			counter = ShowCursor(false);
		} while (counter >= 0);
	}

	cursorVisible = show;
}
