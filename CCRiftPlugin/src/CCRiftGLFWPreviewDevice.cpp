#include "CCRiftGLFWPreviewDevice.h"
#include "DummyPopupView.h"

using namespace std;
using namespace CCRift;

static void HandleRButtonDown(NSWindow* window, float ptx, float pty)
{
    NSRect    graphicsRect;  // contains an origin, width, height
    graphicsRect = NSMakeRect(200, 200, 50, 100);
    //—————————–
    // Create Menu and Dummy View
    //—————————–
    NSMenu* nsMenu = [[[NSMenu alloc] initWithTitle:@"Contextual Menu"] autorelease];
    DummyPopupView* nsView = [[[DummyPopupView alloc] initWithFrame:graphicsRect] autorelease];
    
    NSMenuItem* item = [nsMenu addItemWithTitle:@"Menu Item# 1" action:@selector(OnMenuSelection:) keyEquivalent:@""];
    [item setTarget:nsView];
    [item setTag:1];
    
    item = [nsMenu addItemWithTitle:@"Menu Item #2" action:@selector(OnMenuSelection:) keyEquivalent:@""];
    [item setTarget:nsView];
    [item setTag:2];
    
    //———————————————————————————————
    // Providing a valid windowNumber is key in getting the Menu to display in the proper location
    //———————————————————————————————
    int windowNumber = [(NSWindow*)window windowNumber];
    NSRect frame = [(NSWindow*)window frame];
    
    NSPoint wp = {ptx, frame.size.height - pty};  // Origin in lower left
    NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined
                                        location:wp
                                   modifierFlags:NSApplicationDefined
                                       timestamp: (NSTimeInterval) 0
                                    windowNumber: windowNumber
                                         context: [NSGraphicsContext currentContext]
                                         subtype:0
                                           data1: 0
                                           data2: 0];
    
    [NSMenu popUpContextMenu:nsMenu withEvent:event forView:nsView];
    NSMenuItem* MenuItem = [nsView MenuItem];
    
    switch ([MenuItem tag])
    {
        case 1:
            break;
        case 2:
            break;
    }
    
    
    
}

static void error_callback(int error, const char* description)
{
	//printf(description);
	IDevice<GLFWPreviewDevice>::Instance().glfwErrorCallback(error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	IDevice<GLFWPreviewDevice>::Instance().glfwKeyCallback(window, key, scancode, action, mods);
}

static void close_callback(GLFWwindow* window)
{
	glfwSetWindowShouldClose(window, GL_FALSE);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
	IDevice<GLFWPreviewDevice>::Instance().glfwResizeCallback(window, width, height);
}

GLFWPreviewDevice::GLFWPreviewDevice()
: mWindowSize(glm::ivec2(960, 540))
    , mFrameSize(glm::ivec2(1920, 960))
	, mDeviceRunning(false)
    , mFrameBufferDepth(4)
    , onMouseDownMouseX(0)
    , onMouseDownMouseY(0)
    , lon(0)
    , onMouseDownLon(0)
    , lat(0)
    , onMouseDownLat(0)
    , mMouseSensitivity(0.1f)
    , wasDown(false)
#ifdef CCRIFT_MSW
	, mParentWindow(0)
#endif
    , mAlwaysOnTop(false)
    , mActive(false)
	, verticalFovDegrees(60.0f)
    , contextualMenuCallback([](ContextualMenuOptions){})


	
{
	mFrameBufferLength = mFrameSize.x * mFrameSize.y * mFrameBufferDepth;
	mFrameDataBuffer = new unsigned char[mFrameBufferLength];

	mAspectRatio = (float)mWindowSize.x / (float)mWindowSize.y;
	float verticalFovRadians = verticalFovDegrees * M_PI / 180.0f;
	mProj = glm::perspectiveFovRH(verticalFovRadians, (float)mWindowSize.x, (float)mWindowSize.y, 0.1f, 100.0f); //RH
}

GLFWPreviewDevice::~GLFWPreviewDevice()
{
	this->stop();
	if (mFrameDataBuffer)
		delete[] mFrameDataBuffer;
}

void GLFWPreviewDevice::pushFrame(const void* data)
{
	mFrameDataMutex.lock();

	memcpy(mFrameDataBuffer, (unsigned char*)data, mFrameBufferLength);
	mFrameDataNewFlag = true;

	mFrameDataMutex.unlock();
}

void GLFWPreviewDevice::setActive(bool active)
{
	if (active)
	{
//#ifdef IS_PLUGIN
//#ifdef CCRIFT_MSW
//		HWND h = glfwGetWin32Window(window);
//		SetFocus(h);
//		SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//#endif
//#endif
	}
	else
	{
		
	}

	mActive = active;
}

void GLFWPreviewDevice::start(HINSTANCE hinst)
{
	if (mDeviceRunning) return;

	mDeviceRunning = true;

	//mModuleHandle = hinst;

	mProcess.mThreadCallback = [&]()
	{
		HRESULT hr;
		hr = deviceSetup();

		if (FAILED(hr))
		{
			deviceTeardown();
			mDeviceRunning = false;
			return;
		}

		while (mProcess.mRunning)
		{
			hr = deviceUpdate();

			if (FAILED(hr))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(33));
				continue;
			}
		}

		deviceTeardown();

		mDeviceRunning = false;
	};
    
	mProcess.start();

}

void GLFWPreviewDevice::stop()
{
	if (mProcess.mRunning)
		mProcess.stop();
}


HRESULT GLFWPreviewDevice::deviceSetup()
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
	{
		//MessageBoxA(NULL, "Failed to initialize OpenGL context.", "CCRift Preview", MB_ICONERROR | MB_OK);
		return E_FAIL;
	}

	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(mWindowSize.x, mWindowSize.y, "CCRift Panorama Preview", NULL, NULL);


	if (!window)
	{
		//MessageBoxA(NULL, "Failed to open window.", "CCRift Preview", MB_ICONERROR | MB_OK);
		glfwTerminate();
		return E_FAIL;
	}

	glfwMakeContextCurrent(window);

	//glewExperimental = GL_TRUE;

    if(glewInit() != GLEW_OK)
    {
        //printf("Failed to initialize GLEW\n");
        return E_FAIL;
    }
    
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, size_callback);

#ifdef IS_PLUGIN
	glfwSetWindowCloseCallback(window, close_callback);
	#ifdef CCRIFT_MSW

	HWND h = glfwGetWin32Window(window);
	SetFocus(h);
	SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	mAlwaysOnTop = true;

	//int count;
	//GLFWmonitor** monitors = glfwGetMonitors(&count);

	//if (IsWindow(mParentWindow))// && count >= 2)
	//{
		
		// Only handling the left case
		/*RECT r;
		if(GetWindowRect(mParentWindow, &r))
		{
			glfwSetWindowPos(window, r.right, r.top);
		}*/
		

		/*HMONITOR mNativeParentMonitor = MonitorFromWindow(mParentWindow, MONITOR_DEFAULTTONEAREST);

		for (int i = 0; i < count; i++)
		{
			HMONITOR m = (HMONITOR)(*(monitors + i));
			if(m != mNativeParentMonitor)
			{
				MONITORINFO target;
				target.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(m, &target);
				RECT r = target.rcWork;
				glfwSetWindowPos(window, r.right, r.top);
			}
		}*/

	//}

	
	LONG_PTR style = GetWindowLongPtr(h, GWL_STYLE);
	SetWindowLongPtr(h, GWL_STYLE, style & ~WS_SYSMENU);
	#endif
#endif
	//glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
	
	mScene = new Scene();

	if (!mScene->init(mWindowSize, mFrameSize))
	{
		//MessageBoxA(NULL, "Failed to initialize Scene.", "CCRift Preview", MB_ICONERROR | MB_OK);
		return E_FAIL;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	contextualMenuCallback = [&](ContextualMenuOptions sel){
		if (sel == CONTEXTUAL_MENU_RESET)
		{
			lat = 0;
			lon = 0;
		}
		else if (sel == CONTEXTUAL_MENU_ABOUT)
		{
#ifdef CCRIFT_MSW
			HWND h = glfwGetWin32Window(window);
			MessageBoxA(h, "v0.1\n\nSeptember 2015\n\nandrea.melle@happyfinish.com", "About", MB_ICONINFORMATION | MB_OK);
#endif
		}
		else if (sel == CONTEXTUAL_MENU_GRIDTOGGLE)
		{
			mScene->getSphere()->toggleGrid();

		}
		else if (sel == CONTEXTUAL_MENU_ALWAYSONTOP)
		{
#ifdef IS_PLUGIN
#ifdef CCRIFT_MSW
			HWND h = glfwGetWin32Window(window);
			SetFocus(h);
			if (mAlwaysOnTop)
			{
				SetWindowPos(h, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetWindowPos(h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			else
				SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			mAlwaysOnTop = !mAlwaysOnTop;
#endif
#endif
		}
	};

	// Turn off vsync to let the compositor do its magic
	//wglSwapIntervalEXT(0);

	return S_OK;
}

glm::vec3 GLFWPreviewDevice::handleMouseInput()
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

	//TODO: this totally does not account for mouse going out the window!!!!
	if (state == GLFW_PRESS && !wasDown)
	{
		onMouseDownMouseX = (float)xpos;
		onMouseDownMouseY = (float)ypos;
		onMouseDownLon = lon;
		onMouseDownLat = lat;
		wasDown = true;
	}
	else if (wasDown && state == GLFW_PRESS)
	{
		lon = (onMouseDownMouseX - (float)xpos) * mMouseSensitivity + onMouseDownLon;
		lat = ((float)ypos - onMouseDownMouseY) * mMouseSensitivity + onMouseDownLat;
	}
	else
	{
		wasDown = false;
	}

	float phi = (90.0f - lat) * M_PI / 180.0f;
	float theta = lon * M_PI / 180.0f;

	glm::vec3 result;

	result.x = 500.0f * sin(phi) * cos(theta);
	result.y = 500.0f * cos(phi);
	result.z = 500.0f * sin(phi) * sin(theta);

	state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

	if (state == GLFW_PRESS)
	{
#ifdef CCRIFT_MSW
		HMENU hPopupMenu = CreatePopupMenu();
		InsertMenuW(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, CONTEXTUAL_MENU_RESET, L"Reset");
		
		UINT gridFlags = MF_BYPOSITION | MF_STRING | (mScene->getSphere()->Grid() ? MF_CHECKED : 0);
		InsertMenuW(hPopupMenu, 1, gridFlags, CONTEXTUAL_MENU_GRIDTOGGLE, L"Grid");
#ifdef IS_PLUGIN
		UINT ontopFlags = MF_BYPOSITION | MF_STRING | (mAlwaysOnTop ? MF_CHECKED : 0);
		InsertMenuW(hPopupMenu, 2, ontopFlags, CONTEXTUAL_MENU_ALWAYSONTOP, L"Always On Top");
#endif
		InsertMenuW(hPopupMenu, 3, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
		InsertMenuW(hPopupMenu, 4, MF_BYPOSITION | MF_STRING, CONTEXTUAL_MENU_ABOUT, L"About");

		HWND h = glfwGetWin32Window(window);
		RECT rcWindow, rcClient;
		GetWindowRect(h, &rcWindow);
		GetClientRect(h, &rcClient);

		int ptDiff = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;

		int sel = TrackPopupMenuEx(hPopupMenu,
			TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD,
			rcWindow.left + (int)xpos,
			rcWindow.top + (int)ypos + ptDiff, h, NULL);

		contextualMenuCallback((ContextualMenuOptions)sel);

		DestroyMenu(hPopupMenu);
#else
#ifdef CCRIFT_MAC
        NSWindow* h = glfwGetCocoaWindow(window);
        HandleRButtonDown(h, xpos, ypos);
#endif
#endif
	}

	return result;
}

HRESULT GLFWPreviewDevice::deviceUpdate()
{
	if (!glfwWindowShouldClose(window))
	{
		/*glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;*/

		mFrameDataMutex.lock();
		mScene->updateFrameTexture(mFrameDataBuffer);
		mFrameDataNewFlag = false;
		mFrameDataMutex.unlock();
		
		glm::vec3 target = handleMouseInput();
        glm::mat4 view = glm::lookAtRH(glm::vec3(0, 0, 0), target, glm::vec3(0, 1, 0));

		//glViewport(0, 0, mWindowSize.x, mWindowSize.y);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);

		mScene->render(view, mProj);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	else
	{
		mProcess.mRunning = false;
	}

	return S_OK;
}

HRESULT GLFWPreviewDevice::deviceTeardown()
{
	mScene->release();
	delete mScene;

	glfwDestroyWindow(window);
	glfwTerminate();

	return S_OK;
}

void GLFWPreviewDevice::glfwResizeCallback(GLFWwindow* w, int width, int height)
{
	if (window != w) return;

	mWindowSize.x = width;
	mWindowSize.y = height;

	mAspectRatio = (float)mWindowSize.x / (float)mWindowSize.y;
	float verticalFovRadians = verticalFovDegrees * M_PI / 180.0f;
	mProj = glm::perspectiveFovRH(verticalFovRadians, (float)mWindowSize.x, (float)mWindowSize.y, 0.1f, 100.0f); //RH
	glViewport(0, 0, mWindowSize.x, mWindowSize.y);
}

void GLFWPreviewDevice::glfwKeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
{
#ifndef IS_PLUGIN
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		this->stop();
	}
#endif
}

void GLFWPreviewDevice::glfwErrorCallback(int error, const char* description)
{

}