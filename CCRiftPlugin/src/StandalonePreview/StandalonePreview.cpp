#include <Kernel/OVR_System.h>
#include "OVR_CAPI_GL.h"
#include "CCRiftCommons.h"
#include "CCRiftOGLPlatform.h"
#include "CCRiftScene.h"
#include "CCRiftWinPreviewDevice.h"

using namespace OVR;
using namespace CCRift;

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	IDevice *mDevice = new WinPreviewDevice();

	mDevice->start(hinst);

	size_t demoDataSize = mDevice->preferredFrameHeight()
		* mDevice->preferredFrameWidth()
		* mDevice->preferredFrameDepth();

	unsigned char *demoData = new unsigned char[demoDataSize];

	memset(demoData, 0, demoDataSize);

	while (true)
	{
		if (!mDevice->isRunning())
		{
			mDevice->start(hinst);
		}

		for (int i = 0; i < demoDataSize; i++)
		{
			demoData[i] = rand() % 255;
		}

		mDevice->pushFrame(demoData);
		
		Sleep(5000);
	}

	mDevice->stop();
	delete mDevice;

	return 0;
}