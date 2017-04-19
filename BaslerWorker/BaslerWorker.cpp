// BaslerWorker.cpp : Defines the entry point for the console application.

#undef UNICODE

#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "libconfig.hh"
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
	#include <pylon/PylonGUI.h>
#endif
#include <string>

#pragma comment (lib, "Ws2_32.lib")

using namespace Pylon;
using namespace std;
using namespace libconfig;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 2;
static const size_t c_maxCamerasToUse = 2;

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define VERBOSE

SOCKET init_sock(string port){
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket;
	ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int recvbuflen = DEFAULT_BUFLEN;
	PCSTR def_port;
	def_port = port.c_str();

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, def_port, &hints, &result);
	if (iResult != 0) {
		cerr << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cerr << "socket failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	return ListenSocket;
}

SOCKET accept_socket(string port) {

	SOCKET ClientSocket;
	SOCKET ListenSocket;

	ListenSocket = init_sock(port);

	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		cerr << "accept failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ListenSocket);
	return ClientSocket;
}

string recv_from_socket(SOCKET socket) {
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];

	iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult > 0) {
		#ifdef VERBOSE
		cout << "Bytes received: " << iResult << endl;
		#endif		
	}
	else {

		#ifdef VERBOSE
		cout << "Nothing received" << endl;
		#endif	
		iResult = shutdown(socket, SD_SEND);
	}
	
	return recvbuf;
}

int send_over_socket(SOCKET socket, string buffer_to_send) {
	char sendbuf[128];
	strcpy(sendbuf, buffer_to_send.c_str());
	int iSendResult = send(socket, sendbuf, buffer_to_send.length(), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(socket);
		WSACleanup();
		return -1;
	}
	return iSendResult;
}

int close_socket(SOCKET socket) {
	int iResult;
	iResult = shutdown(socket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed with error: " << WSAGetLastError() << endl;
		closesocket(socket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(socket);
	WSACleanup();
	return 0;
}

int main(int argc, char* argv[])
{
	#pragma region Init
	int exitCode = 0;
	string buffer;
	PylonInitialize(); //inicializacia
	CGrabResultPtr ptrGrabResult;
	bool terminate = false;
	string temp;
	#pragma endregion Init

	//Vytvor socket server a cakaj na pripojenie
	#ifdef VERBOSE
	cout << "Cakam na spojenie s PLC na porte localhost:" << DEFAULT_PORT << endl;	
	#endif
	
	SOCKET main_sock = accept_socket(DEFAULT_PORT); //Tu stoji do pripojenia !!!!

	CTlFactory& tlfactory = CTlFactory::GetInstance();
	DeviceInfoList_t devices; //priprava hladania kamier

	tlfactory.EnumerateDevices(devices);
	if (!devices.empty()) { //ak nie je prazdne pole kamier, ziteruj ich a identifikuj:
		DeviceInfoList_t::const_iterator it;
		for (it = devices.begin(); it != devices.end(); ++it) {
			#ifdef VERBOSE
			cout << it->GetFullName() << "S/N: "<< it->GetSerialNumber()<< " kamera sa nasla!" << endl << endl; //vypis nazov
			temp = std::string(it->GetSerialNumber());
			#endif	
			send_over_socket(main_sock, temp);

		}
	}
	else { //inak vypis chybu
		cerr << "Nenasli sa ziadne kamery!" << endl;
		send_over_socket(main_sock, "err_no_camera_found");

	}

	//vytvor pole kamier (max c_maxcamerastouse)
	CInstantCameraArray kamery(min(devices.size(), c_maxCamerasToUse));

	//napln ich
	for (size_t i = 0; i < kamery.GetSize(); ++i)
	{
		kamery[i].Attach(tlfactory.CreateDevice(devices[i]));
	}

	do {
		buffer = recv_from_socket(main_sock); //nacitaj data zo socketu
		if (buffer == "#exit")break; //ak je prikaz koniec - BREAK

		#ifdef VERBOSE
		cout << "Prijaty kod: "<<buffer << endl;
		#endif

		string serial, filename;
		serial = buffer.substr(0, 8);//skopiruj S/N zo stringu
		filename = buffer.substr(8, buffer.length() - 8); //parse filename


		#ifdef VERBOSE
		cout << "s/n: " << serial << ", filename: " << filename << endl;
		#endif

		for (int i = 0; i < kamery.GetSize(); i++) {
			if(serial.compare(kamery[i].GetDeviceInfo().GetSerialNumber())) {
				cout << "Nasiel som pripojenu kameru: " << serial << "Zachytavam obraz do suboru: " << filename << endl;
				kamery[i].StartGrabbing(1);// Zachyt 1 obrazok


				try {
					while (kamery[i].IsGrabbing())
					{
						// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
						kamery[i].RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

						// Image grabbed successfully?
						if (ptrGrabResult->GrabSucceeded())
						{
							// Access the image data.
							//cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
							//cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
							const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
							//cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

							#ifdef PYLON_WIN_BUILD
							Pylon::DisplayImage(1, ptrGrabResult);	// Display the grabbed image.
							#endif
						}
						else
						{
							cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
							buffer = "#exit";
							terminate = true;
						}
					}
				}catch (const GenericException &e)
				{
					// Error handling.
					cerr << "An exception occurred." << endl
						<< e.GetDescription() << endl;
					exitCode = 1;
				}

				//buffer = "#exit"; //ukonci loop
			}
		}



	} while (buffer!= "#exit");

	cout << "Aplikacia ukoncena vzdialenym klientom kodom: "<< buffer << endl;
		

	// Releases all pylon resources. 
	if(terminate)PylonTerminate();

	return 0;
}