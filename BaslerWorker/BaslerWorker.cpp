// BaslerWorker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "libconfig.hh"


#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
	#include <pylon/PylonGUI.h>
#endif

using namespace Pylon;
using namespace std;
using namespace libconfig;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 2;
static const size_t c_maxCamerasToUse = 2;


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

	//vytvorit a analyzovat dostupne kamery
	CTlFactory& tlfactory = CTlFactory::GetInstance();
	DeviceInfoList_t devices;
	tlfactory.EnumerateDevices(devices);
	if (!devices.empty()) { //ak nie je prazdne pole:
		DeviceInfoList_t::const_iterator it;
		for (it = devices.begin(); it != devices.end(); ++it)
			cout << it->GetFullName() << " found!" << endl << endl; //vypis nazov
	}
	else //inak vypis chybu
		cerr << "No devices found!" << endl;

	/*kamera kamery[8];
	for (unsigned char i; i < 8; i++) {
		kamery[i].camera.Attach(CTlFactory::GetInstance().CreateDevice());
		kamery[i].serial = kamery[i].camera.GetDeviceInfo().GetSerialNumber();
		cout << "found: " << kamery[i].serial << endl;
	}*/


	//vytvor pole kamier (max c_maxcamerastouse)
	CInstantCameraArray kamery(min(devices.size(), c_maxCamerasToUse));

	//napln ich
	for (size_t i = 0; i < kamery.GetSize(); ++i)
	{
		kamery[i].Attach(tlfactory.CreateDevice(devices[i]));
	}

	try
	{
		// Create an instant camera object with the camera device found first.
		//CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		//cout << "Using device " << camera.GetDeviceInfo().GetFullName()<< ", serial: " << camera.GetDeviceInfo().GetSerialNumber() << endl;

		// The parameter MaxNumBuffer can be used to control the count of buffers allocated for grabbing. The default value of this parameter is 10.
		//camera.MaxNumBuffer = 5;

		// Start the grabbing of c_countOfImagesToGrab images. The camera device is parameterized with a default configuration which sets up free-running continuous acquisition.
		kamery[0].StartGrabbing(c_countOfImagesToGrab);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method when c_countOfImagesToGrab images have been retrieved.
		while (kamery[0].IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			kamery[0].RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

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