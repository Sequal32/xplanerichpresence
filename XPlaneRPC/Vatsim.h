#pragma once

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <regex>

std::string *current_data;
char dep[16];
char dest[16];
char callsign[32];

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static bool GetVatsimData(std::string vatsimId) {
	CURL *easyhandle = curl_easy_init();
	std::string readBuffer;

	curl_easy_setopt(easyhandle, CURLOPT_URL, "http://vatsim-data.hardern.net/vatsim-data.txt");
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 2L);
	CURLcode status_code = curl_easy_perform(easyhandle);

	if (status_code != CURLE_OK) return false;

	char match_string[64];
	sprintf(match_string, "[^\n]%s[^\n]+", vatsimId);

	std::smatch m;
	std::regex r(match_string);
	
	regex_search(readBuffer, m, r);
	std::string result = m[0];

	if (result == "") return false;

	std::smatch m1;
	std::regex r1(":([A-Z]{4}):");

	regex_search(result, m1, r1);
	std::string t_dep = m1[1];
	sprintf(dep, "%s", t_dep);
	std::string s = m1.suffix().str();
	regex_search(s, m1, r1);
	std::string t_dest = m1[1];
	
	sprintf(dest, "%s", t_dest);

	sprintf(callsign, "%s", result.substr(0, result.find(':')));

	return true;
}