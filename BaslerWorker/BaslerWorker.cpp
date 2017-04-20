/* BaslerWorker.cpp : Defines the entry point for the console application.
Created under GNU/GPL license v3
Author: Stanislav Rubint, Ing., www.rubint.sk, 2017
*/

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\video\video.hpp>
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
	#include <pylon/PylonGUI.h>
#endif
#include <string>
#include <fstream>

#pragma comment (lib, "Ws2_32.lib")

using namespace Pylon;
using namespace std;
using namespace cv;

static const size_t c_maxCamerasToUse = 8;
static bool connection_alive = false;
static bool run_program = true;

//#define VERBOSE
#define DEFAULT_BUFLEN 256
#define CONFIG_FILE "defaults.cfg"

static string DEFAULT_PORT = "27015";
static string EXIT_KEY = "#exit";
static string CLOSE_KEY = "#close";
static string IMAGE_PATH = "";

static string SN_NOT_FOUND = "err_sn_not_found;";
static string NO_CAMERA_FOUND = "err_no_camera_found;";
static string CAMERA_FOUND = "camera_found;";
static string CAPTURE_FAILED = "err_capture_failed;";
static string SAVING_FAILED = "err_image_not_saved;";
static string IMG_SAVED = "image_saved_succesfully;";


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
		return -1;
		connection_alive = false;
	}
	closesocket(ListenSocket);
	connection_alive = true;
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
		recvbuf[iResult] = 0;
	}
	else {
		#ifdef VERBOSE
		cout << "Nothing received" << endl;
		#endif	
		iResult = shutdown(socket, SD_SEND);
		string temp = CLOSE_KEY;
		strcpy_s(recvbuf, temp.c_str());
		connection_alive = false;
	}
	
	return recvbuf;
}

int send_over_socket(SOCKET socket, string buffer_to_send) {
	int iSendResult = -1;
	if (connection_alive) {
		char sendbuf[128];
		strcpy_s(sendbuf, buffer_to_send.c_str());
		iSendResult = send(socket, sendbuf, (int)buffer_to_send.length(), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(socket);
			WSACleanup();
			connection_alive = false;
			return -1;
		}
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

	closesocket(socket);
	WSACleanup();
	connection_alive = false;
	return 0;
}

string parse_parameter(string file, string parameter) {
	ifstream subor;
	string value = "";
	string line, prm, candidate;
	int val_pos;
	subor.open(CONFIG_FILE);

	while (subor.is_open() && getline(subor, line)) {
		val_pos = (int)line.find_first_of('=');
		if (val_pos != string::npos && line[0]!='#') {
			candidate = line.substr(val_pos + 1, (int)line.length() - val_pos - 1);
			prm = line.substr(0, val_pos);
			if (prm.compare(parameter) == 0) {
				value = candidate;
				break;
			}
		}
	}
	subor.close();
	return value;
}

void load_global_parameters() {
	string temp;
	temp = parse_parameter(CONFIG_FILE, "PORT");
	if (temp.compare("")) {
		DEFAULT_PORT = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "EXIT_KEY");
	if (temp.compare("")) {
		EXIT_KEY = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "CLOSE_KEY");
	if (temp.compare("")) {
		CLOSE_KEY = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "IMAGE_PATH");
	if (temp.compare("")) {
		IMAGE_PATH = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "SN_NOT_FOUND");
	if (temp.compare("")) {
		SN_NOT_FOUND = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "NO_CAMERA_FOUND");
	if (temp.compare("")) {
		NO_CAMERA_FOUND = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "CAMERA_FOUND");
	if (temp.compare("")) {
		CAMERA_FOUND = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "CAPTURE_FAILED");
	if (temp.compare("")) {
		CAPTURE_FAILED = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "SAVING_FAILED");
	if (temp.compare("")) {
		SAVING_FAILED = temp;
	}
	temp = parse_parameter(CONFIG_FILE, "IMG_SAVED");
	if (temp.compare("")) {
		IMG_SAVED = temp;
	}
}

int main(int argc, char* argv[])
{
	#pragma region Init
	int exitCode = 0;
	string buffer;
	PylonInitialize();										//inicializacia kamier a ost.
	CGrabResultPtr ptrGrabResult;
	bool terminate = false;
	string temp;
	SOCKET main_sock;
	bool camera_found = false;
	Mat	opencvImage;
	CPylonImage pylonImage;
	CImageFormatConverter formatConverter;
	#pragma endregion Init
	
	load_global_parameters();

	while (run_program) {									//po odpojeni socketu cakaj na dalsi, zober fotky a tak dookola...

#ifdef VERBOSE
		cout << "Cakam na spojenie s PLC na porte localhost:" << DEFAULT_PORT << endl;
#endif

		main_sock = accept_socket(DEFAULT_PORT);			//Vytvor socket server a cakaj na pripojenie, pozor, tu stoji do pripojenia !!!!
		if (main_sock == -1) {
			run_program = false;
			cerr << "unable to open socket" << endl;
			break;
		}
		CTlFactory& tlfactory = CTlFactory::GetInstance();
		DeviceInfoList_t devices;							//priprava hladania kamier

		tlfactory.EnumerateDevices(devices);
		if (!devices.empty()) {								//ak nie je prazdne pole kamier, ziteruj ich a identifikuj:
			DeviceInfoList_t::const_iterator it;
			for (it = devices.begin(); it != devices.end(); ++it) {
#ifdef VERBOSE
				cout << it->GetFullName() << "S/N: " << it->GetSerialNumber() << " kamera sa nasla!" << endl << endl; //vypis nazov
#endif	
				temp = string(it->GetSerialNumber());
				temp.insert(0, "C_");
				temp.append(",IP_");
				temp.append(string(it->GetFullName()),33, string(it->GetFullName()).find_first_of(':')-string(it->GetFullName()).find_last_of('#')-1);
				temp.append(";");
				send_over_socket(main_sock, temp);			//urob cary/mary, pridaj S/N kamery a IP adresu a posli cez socket

			}
		}
		else { //inak vypis chybu
			cerr << "Nenasli sa ziadne kamery!" << endl;
			send_over_socket(main_sock, NO_CAMERA_FOUND);

		}
		CInstantCameraArray kamery(min(devices.size(), c_maxCamerasToUse));				//vytvor pole kamier (max c_maxcamerastouse)

		for (size_t i = 0; i < kamery.GetSize(); ++i)		//napln ich dostupnymi kamerami
		{
			kamery[i].Attach(tlfactory.CreateDevice(devices[i]));
		}

		do {
			buffer = recv_from_socket(main_sock);			//nacitaj data zo socketu
			if (buffer == CLOSE_KEY)break;					//ak je prikaz koniec	- zavri kamery aj Socket
			if (buffer == EXIT_KEY) {						//ak je prikaz EXIT_KEY	- zavri a opust program
				run_program = false;
				break;
			}

#ifdef VERBOSE
			cout << "Prijaty kod: " << buffer << endl;
#endif

			string serial, filename;
			serial = buffer.substr(0, 8);												//skopiruj S/N zo stringu
			filename = buffer.substr(8, buffer.length() - 8);
			filename.insert(0, IMAGE_PATH);												//aj nazov suboru
			buffer = "";																//zma� buffer

#ifdef VERBOSE
			cout << "s/n: " << serial << ", filename: " << filename << endl;
#endif
			camera_found = false;
			for (int i = 0; i < kamery.GetSize(); i++) {
				temp = std::string(kamery[i].GetDeviceInfo().GetSerialNumber());
				if (!temp.compare(serial)) {
					camera_found = true;
					send_over_socket(main_sock, CAMERA_FOUND);
#ifdef VERBOSE
					cout << "Nasiel som pripojenu kameru: " << serial << endl<<"Zachytavam obraz do suboru: " << filename << endl;
#endif
					kamery[i].StartGrabbing(1);				// Zachyt 1 obrazok
					terminate = true;

					try {
						while (kamery[i].IsGrabbing())
						{		
							kamery[i].RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);	// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
							if (ptrGrabResult->GrabSucceeded())												//Zachytilo obrazok uspesne?
							{
								// Access the image data.
								//cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
								//cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
								const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
								//cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

#ifdef PYLON_WIN_BUILD
#ifdef VERBOSE
								Pylon::DisplayImage(1, ptrGrabResult);		// Display the grabbed image.
#endif
#endif	
								formatConverter.OutputPixelFormat = PixelType_BGR8packed;
								formatConverter.Convert(pylonImage, ptrGrabResult);

								try {										//sk�s ulo�i� obr�zok
									opencvImage = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());
									if (!imwrite(filename, opencvImage)) {
										send_over_socket(main_sock, SAVING_FAILED);
										cerr << "Error saving image: " << SAVING_FAILED << endl;
									}
									else {
										send_over_socket(main_sock, IMG_SAVED);
									}
								}
								catch (const GenericException &e) {
									cerr << e.GetDescription() << endl << "Line: " << e.GetSourceLine() << endl;
									break;
								}

							}
							else
							{
								cerr << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
								buffer = CLOSE_KEY;
								send_over_socket(main_sock, CAPTURE_FAILED);
							}
						}
					}
					catch (const GenericException &e)
					{
						cerr << "An exception occurred." << endl << e.GetDescription() << endl;
						send_over_socket(main_sock, "err_capt_image");
						exitCode = 1;
					}
				}
			}
			if (!camera_found) {
				cerr << SN_NOT_FOUND << endl;
				send_over_socket(main_sock, SN_NOT_FOUND);
			}

		} while (buffer != CLOSE_KEY);		//koniec hlavnej slu�ky pr�jmu d�t

#ifdef VERBOSE
		cout << "Aplikacia ukoncena vzdialenym klientom kodom: " << buffer << endl;
#endif

		if (connection_alive) {
			close_socket(main_sock);
		}
		// Releases all pylon resources. 
		//PylonTerminate(); // nefunguje !!!
	}	//koniec slu�ky programu

	return 0;
}