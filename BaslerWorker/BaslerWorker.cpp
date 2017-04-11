// BaslerWorker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "libconfig.hh"


#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

using namespace Pylon;
using namespace std;
using namespace libconfig;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 2;

struct kamera {
	CInstantCamera camera;
	unsigned long exposure;
	unsigned char gain;
	string ID;
	string serial;

};

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized. 
	PylonInitialize();
	kamera kamery[8];
	for (unsigned char i; i < 8; i++) {
		kamery[i].camera.Attach(CTlFactory::GetInstance().CreateFirstDevice());
	}

	try
	{
		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetFullName()<< ", serial: " << camera.GetDeviceInfo().GetSerialNumber() << endl;

		// The parameter MaxNumBuffer can be used to control the count of buffers allocated for grabbing. The default value of this parameter is 10.
		camera.MaxNumBuffer = 5;

		// Start the grabbing of c_countOfImagesToGrab images. The camera device is parameterized with a default configuration which sets up free-running continuous acquisition.
		camera.StartGrabbing(c_countOfImagesToGrab);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{
				// Access the image data.
				//cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
				//cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
				const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
				//cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

				#ifdef PYLON_WIN_BUILD
				// Display the grabbed image.
					Pylon::DisplayImage(1, ptrGrabResult);
				#endif
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
	}
	catch (const GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	// Releases all pylon resources. 
	PylonTerminate();

	return exitCode;
}