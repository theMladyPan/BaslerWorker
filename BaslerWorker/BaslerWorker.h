#undef UNICODE
#define WIN32_LEAN_AND_MEAN

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
#include <algorithm>	
#include <string>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

using namespace Pylon;
using namespace std;
using namespace cv;
using namespace GenApi;

#pragma region defaults
static const size_t c_maxCamerasToUse = 8;
static bool connection_alive = false;
static bool run_program = true;

#define DEFAULT_BUFLEN 256
#define CONFIG_FILE "defaults.cfg"

static string DEFAULT_PORT = "49000";
static string EXIT_KEY = "<exit>";
static string CLOSE_KEY = "<close>";
static string IMAGE_PATH = "";
static string TIMEOUT = "5000"; //ms

static string SN_NOT_FOUND = "err_sn_not_found;";
static string NO_CAMERA_FOUND = "err_no_camera_found;";
static string CAMERA_FOUND = "camera_found;";
static string CAPTURE_FAILED = "err_capture_failed;";
static string SAVING_FAILED = "err_image_not_saved;";
static string IMG_SAVED = "image_saved_succesfully;";
static string INVALID_FILENAME = "err_invalid_filename;";
static string MSG_SHORT = "err_message_too_short_or_invalid";
static string EXIT_MSG = "exitting...;";
static string CONN_BROKEN = "err_connection_to_camera_broken;";
#pragma endregion //defaults

SOCKET init_sock(string port);
SOCKET accept_socket(string port);
string recv_from_socket(SOCKET socket);
int send_over_socket(SOCKET socket, string buffer_to_send);
int close_socket(SOCKET socket);
string parse_parameter(ifstream &subor, string parameter);
void load_global_parameters(ifstream &subor);


boolean verbose = false;