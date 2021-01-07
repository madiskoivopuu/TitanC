#include "mainfuncs.h"
#include "Titan.h"

#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <cctype>
#include <map>
#include <array>
#include <algorithm>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include <random>
#include <iterator>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#ifdef __linux__
#include <unistd.h>
#define WIN_MACHINE 0
#endif
#ifdef _WIN32
#define WIN_MACHINE 1
#include <windows.h>
#endif

#define CURRENT_VERSION "1.0.0"
rapidjson::Document settings;
bool bWhitelistValidated = false;
int failedValidations = 0;

std::vector<std::string> cookies;
std::vector<std::string> pricecheckCookies;
std::vector<std::string> proxies;
std::map<std::string, std::string> cachedTokens;

/* GLOBAL NON-SNIPER FUNCTIONS */

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static size_t noop(void* contents, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;
}

bool isStringNumber(std::string s) {
	for (size_t i = 0; i < s.size(); ++i) {
		if (!std::isdigit(s[i])) return false;
	}
	return true;
}

void eraseSubStr(std::string& mainStr, const std::string& toErase)
{
	// Search for the substring in string
	size_t pos = mainStr.find(toErase);

	if (pos != std::string::npos)
	{
		// If found then erase it from string
		mainStr.erase(pos, toErase.length());
	}
}

void sleepcp(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
	Sleep(milliseconds);
#else
	usleep(milliseconds * 1000);
#endif
}

std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

// linux exec

/*std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}*/


/* GLOBAL SNIPER FUNCTIONS */

std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter)
{
	std::vector<std::string> splittedString;
	size_t startIndex = 0;
	size_t  endIndex = 0;
	while ((endIndex = stringToBeSplitted.find(delimeter, startIndex)) < stringToBeSplitted.size())
	{
		std::string val = stringToBeSplitted.substr(startIndex, endIndex - startIndex);
		splittedString.push_back(val);
		startIndex = endIndex + delimeter.size();
	}
	if (startIndex < stringToBeSplitted.size())
	{
		std::string val = stringToBeSplitted.substr(startIndex);
		splittedString.push_back(val);
	}
	return splittedString;
}

std::map<std::string, std::string, CaseInsensitive> parse_headers(std::string str) {
	std::vector<std::string> headerVector = split(str, "\r\n");
	std::map<std::string, std::string, CaseInsensitive> headers;

	for (size_t i = 0; i < headerVector.size(); ++i) {
		std::vector<std::string> temp = split(headerVector[i], ":");
		if (temp.size() > 1)
		{
			headers[temp[0]] = temp[1].erase(0, temp[1].find_first_not_of("\r\n\t "));
		}
	}

	return headers;
}

void addCookie(std::string cookie) { // Adds cookie to token caching list
	if (std::find(cookies.begin(), cookies.end(), cookie) == cookies.end())
	{
		cookies.push_back(cookie);
	}
}

std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	str.erase(str.find_last_not_of(chars) + 1);
	return str;
}

bool validateSettings() {
	if (!settings.HasMember("webhook"))
	{
		std::cout << "ERROR: Config.cfg is missing \"webhook\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("webhookMention"))
	{
		std::cout << "ERROR: Config.cfg is missing \"webhookMention\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("MinProfitMargin"))
	{
		std::cout << "ERROR: Config.cfg is missing \"MinProfitMargin\" field.Please check #sniper - config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("EnableTryingSnipePrint"))
	{
		std::cout << "ERROR: Config.cfg is missing \"EnableTryingSnipePrint\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("FailedSnipeWebhookSpamBlockTimeInSeconds"))
	{
		std::cout << "ERROR: Config.cfg is missing \"FailedSnipeWebhookSpamBlockTimeInSeconds\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("EnableFailedSnipeNotifications"))
	{
		std::cout << "ERROR: Config.cfg is missing \"EnableFailedSnipeNotifications\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("AutoSell"))
	{
		std::cout << "ERROR: Config.cfg is missing \"AutoSell\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("MoveToNextAccountWhenRobuxUnderCertainThreshold"))
	{
		std::cout << "ERROR: Config.cfg is missing \"MoveToNextAccountWhenRobuxUnderCertainThreshold\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("MinRobuxThreshold"))
	{
		std::cout << "ERROR: Config.cfg is missing \"MinRobuxThreshold\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("accounts"))
	{
		std::cout << "ERROR: Config.cfg is missing \"accounts\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	if (!settings.HasMember("Amplify"))
	{
		std::cout << "ERROR: Config.cfg is missing \"Amplify\" field. Please check #sniper-config in our discord." << std::endl;
		return false;
	}

	rapidjson::Value& accounts = settings["accounts"];
	if (accounts.IsArray())
	{
		for (rapidjson::SizeType i = 0; i < accounts.Size(); i++) {
			if (!accounts[i].HasMember("cookie"))
			{
				std::cout << "ERROR: Config.cfg is missing \"cookie\" field in \"accounts\". Please check #sniper-config in our discord." << std::endl;
				return false;
			}
		}
	}
	else
	{
		std::cout << "ERROR: \"accounts\" field in settings must be a list. (\"accounts\": [{...}, {...}])." << std::endl;
		return false;
	}

	return true;
}
