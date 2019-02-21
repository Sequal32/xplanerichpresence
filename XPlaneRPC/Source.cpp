/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <windows.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>

#include "XSBComDefs.h"
#include "XPLMDataAccess.h"
#include "XPLMNavigation.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include "Discord.h"
#include "json.hpp"
#include "Vatsim.h"
using namespace std;
using json = nlohmann::json;

float LoopCallback(float elaspedLastCall, float elaspedLastLoop, int counter, void *refcon);

//Defining DataRefs
XPLMDataRef lat;
XPLMDataRef lon;
XPLMDataRef gndspeed;
XPLMDataRef alt;
XPLMDataRef agl;
XPLMDataRef status;
XPLMDataRef aircraftType;

XPLMDataRef pilot_id;

//Squawkbox
XPLMPluginID sqPlugin;
bool usingSquawkbox = false;
std::string vatsimId;
 

//Flightplan
char c_dep[16];
char c_dest[16];
float dest_lon;
float dest_lat;

//Current Aircraft
char current_acficao[40];
char lower_acficao[40];

//Get aircraft types from JSON file
//ifstream i("aircrafttypes.json");
//json j;
//i >> j;

PLUGIN_API int XPluginStart(char * outName, char * outSignature, char * outDescription) {
	strcpy(outName, "TestPlugin");
	strcpy(outSignature, "sequal.discordrpc");
	strcpy(outDescription, "Discord rich presence for XPlane");
    
	InitDiscord();

	//FindingDataRefs
	lat = XPLMFindDataRef("sim/flightmodel/position/latitude");
	lon = XPLMFindDataRef("sim/flightmodel/position/longitude");
	gndspeed = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
	alt = XPLMFindDataRef("sim/flightmodel/position/elevation");
	agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");
	aircraftType = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
	pilot_id = XPLMFindDataRef("xsquawkbox/login/pilot_id");

	return 1;
}

string LookupAircraftType(string icao) {

	//return aircraftTypes[icao];
}

void GetVatsimId() {
	ifstream file;
	file.open("vatsimid.txt");
	if (file.is_open())
		getline(file, vatsimId);
	else
		vatsimId = "";
	file.close();
}

char *destinationICAO;

int totaltime = 0;

float LoopCallback(float elaspedLastCall, float elaspedLastLoop, int counter, void *refcon) {
	string state;
	double aglVal = XPLMGetDataf(agl)*3.28084;
	double gndVal = XPLMGetDataf(gndspeed)*1.94384449411997;
	double altVal = XPLMGetDataf(alt)*3.28084;

	double distance;
	
	boolean onGround = (aglVal <= 100 && gndVal <= 25) ? true : false;

	//Finding Flight Plan
	if (totaltime % 30 == 0 and usingSquawkbox and vatsimId != "") {
		if (GetVatsimData(vatsimId)) {
			if (dest != c_dest) {
				float *p_dest_lat = &dest_lat;
				float *p_dest_lon = &dest_lon;
				XPLMNavRef dest_ref = XPLMFindNavAid(NULL, dest, NULL, NULL, NULL, xplm_Nav_Airport);
				XPLMGetNavAidInfo(dest_ref, NULL, p_dest_lat, p_dest_lon, NULL, NULL, NULL, NULL, NULL, NULL);
				strcpy(c_dest, dest);
			}
		}
		else {
			strcpy(dep, "");
			strcpy(dest, "");
			dest_lat = NULL;
			dest_lon = NULL;
			distance = NULL;
		}
	}
	if (usingSquawkbox and dest_lat != NULL)
		distance = ceil(abs(XPLMGetDatad(lat) * 60.063638 - dest_lat * 60.063638) + abs(XPLMGetDatad(lon) * 60.063638 - dest_lon * 60.107087));
	else
		distance = NULL;

	//Updating Discord Presence

	char dstate[128];
	char details[256];
	char flight_state[32];

	int timestamp;

	sprintf(dstate, "%d ft %d kts", (int)altVal, (int)gndVal);

	if (distance != NULL) {
		sprintf(details, "%s - %s", dep, dest);
		if (gndVal > 50) timestamp = (distance / gndVal * 3600) + std::time(NULL); else timestamp = NULL;
		UpdatePresence(dstate, details, timestamp, NULL, current_acficao, "vatsim", callsign);
	}
	else {
		if (onGround) {
			if (gndVal > 5)
				sprintf(details, "Taxiing");
			else
				sprintf(details, "On Ground");
		}
		else
			sprintf(details, "In Flight");
		UpdatePresence(dstate, details, NULL, NULL, current_acficao, NULL, NULL);
	}
	totaltime++;
    return 10.0;
}

PLUGIN_API void	XPluginStop(void) {
	//XPLMDestroyWindow(g_window);
	//g_window = NULL;
}

PLUGIN_API void XPluginDisable(void) {
	XPLMUnregisterFlightLoopCallback(LoopCallback, NULL);
}
PLUGIN_API int  XPluginEnable(void)  { 
	XPLMRegisterFlightLoopCallback(LoopCallback, 1, NULL);
	sqPlugin = XPLMFindPluginBySignature("vatsim.protodev.clients.xsquawkbox"); //Finding the sqawkbox plugin

	//Find user aircraft info
	XPLMGetDatab(aircraftType, current_acficao, 0, sizeof(current_acficao));
	GetVatsimId();

	for (int i = 0; i < strlen(current_acficao); i++)
		lower_acficao[i] = tolower(current_acficao[i]);

	if (sqPlugin != XPLM_NO_PLUGIN_ID) { // If found
		usingSquawkbox = true;
		//XPLMSendMessageToPlugin(sqPlugin, XSB_CMD_SUBSCRIBE, (void *) (XSB_NETWORK));
		//XPLMGetDatab(pilot_id, vatsimId, 0, sizeof(pilot_id));
	}
	return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) { 
	XPLMDebugString("RPC: Recieved message: " + inMsg);
	switch (inMsg) {
		case XSB_MSG_CONNECTED:
			//XPLMGetDatab(pilot_id, vatsimId, 0, sizeof(pilot_id));
			//XPLMDebugString("RPC: Connection to VATSIM detected Pilot ID: " + *vatsimId);
			usingSquawkbox = true;
			break;
		case XSB_MSG_DISCONNECTED:
			XPLMDebugString("RPC: Disconnected from VATSIM");
			usingSquawkbox = false;
			break;
	}
}

int main() {
	return 1;
}