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

bool validateVersion() {
	CURL* curl = curl_easy_init();
	CURLcode res;
	std::string version;
	curl_easy_setopt(curl, CURLOPT_URL, "https://vortex-b.xyz/api/ascensionsniper/version");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &version);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res == CURLE_OK)
	{
		if (version == CURRENT_VERSION)
		{
			return true;
		}
		else
		{
			std::cout << "There is a newer version available for Titan, please download that." << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "Failed to validate version due to a request error, please re-open the program." << std::endl;
		return false;
	}
}

bool validateWhitelist() {
	std::ifstream file;
	std::string fileContent;

	try
	{
		file.open("./Settings/Whitelist.txt", std::ifstream::in);
		if (!file.is_open())
		{
			std::cout << "<=> Failed to open Whitelist.txt, make sure it exsists." << std::endl;
			bWhitelistValidated = false;
			return bWhitelistValidated;
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		fileContent = content;
		file.close();
	}
	catch (const std::system_error & e)
	{
		std::cout << "<=> Failed to read content from Whitelist.txt" << std::endl;
		std::clog << e.what() << " (" << e.code() << ")" << std::endl;
		return false;
	}

	// Check if there are no details in Whitelist.txt
	if (fileContent.length() == 0)
	{
		std::cout << "<=> You have no whitelist details in Whitelist.txt." << std::endl;
		return false;
	}

	// Check if you can't split the string into uname&pw
	int pos = fileContent.find_first_of(':');
	if (!pos)
	{
		std::cout << "<=> Missing \":\" in Whitelist.txt, exiting." << std::endl;
		return false;
	}
	std::string password = fileContent.substr(pos + 1), username = fileContent.substr(0, pos);

	// Get hwid
	std::string hwid;
	std::string delimiter = "\n";
	if (WIN_MACHINE)
	{
		try
		{
			hwid = exec("wmic csproduct get uuid");
			int pos2 = 0;
			bool firstIter = true;
			while ((pos2 = hwid.find(delimiter)) != std::string::npos) {
				if (!firstIter)
				{
					hwid = hwid.substr(0, pos2);
					break;
				}
				hwid.erase(0, pos2 + delimiter.length());
				firstIter = false;
			}
		}
		catch (const std::exception & e)
		{
			std::cout << "<=> Failed to get HWID (#1)." << std::endl;
			std::clog << e.what() << std::endl;
			return false;
		}
	}
	else
	{
		try
		{
			if (!std::experimental::filesystem::exists("/etc/uuid"))
			{
				system("cat /proc/sys/kernel/random/uuid >/etc/uuid");
			}

			std::ifstream hwidFile;
			hwidFile.open("/etc/uuid", std::ifstream::in);

			if (!hwidFile.is_open())
			{
				std::cout << "<=> Failed to get HWID (#2)." << std::endl;
				return false;
			}

			std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			hwid = content;
			hwidFile.close();
		}
		catch (const std::exception & e)
		{
			std::cout << "<=> Failed to get HWID (#1)." << std::endl;
			std::clog << e.what() << std::endl;
			return false;
		}
	}

	// Make necessary post requests
	CURL* curl = curl_easy_init();
	CURLcode res;
	int http_code = 0;
	if (curl)
	{
		// Send 1st post
		curl_easy_setopt(curl, CURLOPT_URL, "https://vortex-b.xyz/api/ascensionsniper/bool/killswitch");
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
		{
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200)
			{
				// reset curl
				curl_easy_reset(curl);
				std::string postfields1 = "username=" + username + "&hwid=" + rtrim(hwid);

				// send 2nd post
				curl_easy_setopt(curl, CURLOPT_URL, "https://vortex-b.xyz/api/ascensionsniper/login/hwid");
				curl_easy_setopt(curl, CURLOPT_POST, 1);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields1.c_str());
				curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postfields1.length());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
				res = curl_easy_perform(curl);
				if (res == CURLE_OK)
				{
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
					if (http_code == 200)
					{
						// reset curl
						curl_easy_reset(curl);
						std::string postfields2 = "username=" + username + "&password=" + password;
						std::string response;

						// send 3rd post
						curl_easy_setopt(curl, CURLOPT_URL, "https://vortex-b.xyz/api/ascensionsniper/login/whitelist");
						curl_easy_setopt(curl, CURLOPT_POST, 1);
						curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields2.c_str());
						curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postfields2.length());
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
						curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
						res = curl_easy_perform(curl);
						if (res == CURLE_OK)
						{
							curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
							if (http_code == 200)
							{
								curl_easy_cleanup(curl);

								rapidjson::Document jsonData;
								jsonData.Parse(response.c_str());
								if (jsonData.HasParseError())
								{
									std::cout << "<=> FATAL ERROR: Price check fix failed, try restarting the program. If the same error keeps ocurring send a PM to Sparkly." << std::endl;
									return false;
								}
								for (rapidjson::Value::ConstValueIterator itr = jsonData.Begin(); itr != jsonData.End(); ++itr)
								{
									pricecheckCookies.push_back((*itr).GetString());
								}

								return true;
							}
							else
							{
								curl_easy_cleanup(curl);
								std::cout << "<=> Failed to authenticate. (#3)" << std::endl;
								return false;
							}
						}
						else
						{
							curl_easy_cleanup(curl);
							std::cout << "<=> Failed to authenticate. Request error (#3): " << res << std::endl;
							return false;
						}
					}
					else
					{
						curl_easy_cleanup(curl);
						std::cout << "<=> Failed to authenticate. (#2)" << std::endl;
						return false;
					}
				}
				else
				{
					curl_easy_cleanup(curl);
					std::cout << "<=> Failed to authenticate. Request error (#2): " << res << std::endl;
					return false;
				}

			}
			else
			{
				curl_easy_cleanup(curl);
				std::cout << "<=> Failed to authenticate. (#1)" << std::endl;
				return false;
			}
		}
		else
		{
			curl_easy_cleanup(curl);
			std::cout << "<=> Failed to authenticate. Request error (#1): " << res << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "<=> Failed to authenticate with whitelist (failed to initialize requests)." << std::endl;
		return false;
	}

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