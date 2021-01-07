// app3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Titan.h"
#include "mainfuncs.h"
#include "CSniperThread.h"
#include "CTokenCacher.h"
#include "rblx.h"

#include <iostream>
#include <thread>
#include <cstdlib>
#include <curl/curl.h>
#include <string>
#include <chrono>
#include <fstream>

/*
#include <sys/types.h>
#include <io.h>
#include <Windows.h>
#include <tchar.h>
#include <winnt.h>
#include <winternl.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <process.h>*/

/* Connect static libraries
#ifdef WIN32
#pragma comment (lib, "libcurl.lib")
#pragma comment (lib, "wldap32.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "crypt32.lib")
#endif*/

bool g_bIsProgramRunning = true;

int main()
{
	SetConsoleTitle(TEXT("Titan"));

	// Parse JSON
	std::ifstream file, idsFile;
	std::string fileContent;
	std::vector<std::string> assetIds;
	try
	{
		file.open("./Settings/Config.cfg", std::ifstream::in);
		if (!file.is_open())
		{
			std::cout << "<=> Failed to open Config.cfg, make sure it exsists and is readable by the program (read access)." << std::endl;
			std::cin.get();
			return 0;
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		fileContent = content;
		file.close();
	}
	catch (const std::exception & e)
	{
		std::cout << "<=> Failed to read content from Config.cfg" << std::endl;
		std::clog << e.what() << std::endl;
		std::cin.get();
		return 0;
	}

	settings.Parse(fileContent.c_str());
	if (settings.HasParseError())
	{
		std::cout << "<=> Failed to parse json from Config.cfg. Check your config at jsonlint.com." << std::endl;
		std::cin.get();
		return 0;
	}

	// Validate version & login

	if (validateSettings() == false)
	{
		std::cin.get();
		return 0;
	}


	else bWhitelistValidated = true;

	std::cout << "<=> No whitelist validation needed | Fixed by Bixmox#2482 in 5 seconds." << std::endl;
	std::cout << "<=> Titan was made by Sparkly (https://github.com/SparklyCatTF2/TitanC)" << std::endl;
	std::cout << "<=> Fuck the people who tried selling the 'fix'\n" << std::endl;

	// Open asset IDs file
	try
	{
		idsFile.open("./Settings/asset_ids.txt", std::ifstream::in);
		if (!idsFile.is_open())
		{
			std::cout << "<=> Failed to open asset_ids.txt, make sure it exsists and is readable by the program (read access)." << std::endl;
			std::cin.get();
			return 0;
		}

		std::string line;
		while (idsFile >> line) //std::getline(idsFile, line, '\n')
		{
			if (!isStringNumber(line))
			{
				continue;
			}
			else
			{
				assetIds.push_back(line);
			}
		}

		idsFile.close();
	}
	catch (const std::exception & e)
	{
		std::cout << "<=> Failed to read content from asset_ids.txt" << std::endl;
		std::clog << e.what() << std::endl;
		std::cin.get();
		return 0;
	}

	// check for assetids, if there are none ask the user to input some
	if (assetIds.size() < 1)
	{
		std::cout << "<=> Cannot snipe without any asset IDs, please add some (enter 'DONE' to finish)." << std::endl;
		while (true) {
			std::string id;
			std::cout << "<=> Enter an ID: ";
			std::getline(std::cin, id);

			if (id == "DONE" || id == "done")
			{
				if (assetIds.size() < 1)
				{
					std::cout << "<=> Cannot snipe without any asset IDs, please add some (enter 'DONE' to finish)." << std::endl;
					continue;
				}

				break;
			}

			if (!isStringNumber(id))
			{
				std::cout << "<=> Your ID '" << id << "' is not a number." << std::endl;
			}
			else
			{
				assetIds.push_back(id);
			}
		}
	}

	std::cout << "<=> Asset IDs loaded (" << assetIds.size() << ")" << std::endl;

	// Get IDs per thread
	int idsPerThread = 0;
	while (true)
	{
		std::string id;
		std::cout << "<=> IDs per thread: ";
		std::getline(std::cin, id);

		try
		{
			unsigned int num = std::stoi(id);
			if (num == 0)
			{
				std::cout << "<=> IDs per thread must be a number" << std::endl;
			}
			else
			{
				if (num > assetIds.size()) {
					idsPerThread = assetIds.size();
					std::cout << "<=> WARNING: IDs per thread is smaller than the amount of asset IDs, setting IDs per thread to " << assetIds.size() << std::endl;
				}
				else {
					idsPerThread = num;
				}
				break;
			}
		}
		catch (const std::exception & e)
		{
			std::cout << "<=> IDs per thread is too large." << std::endl;
			std::clog << e.what() << std::endl;
		}
	}

	int threadNum = 1, currentAssetNum = 1;
	std::vector<std::string> passAssetIds;
	CTokenCacher tk;

	// start checker threads
	std::thread tokenCacher(&CTokenCacher::cacheTokens, tk);
	tokenCacher.detach();

	// log in to roblox accounts and save rblx sessions to a vector
	std::vector<Roblox> rblx_sessions;
	int successful_logins = 0;

	std::cout << "<=> Trying to log into snipe accounts" << std::endl;
	for (rapidjson::Value::ConstValueIterator itr = settings["accounts"].Begin(); itr != settings["accounts"].End(); ++itr) {
		Roblox rblx;
		std::string cookie = (*itr)["cookie"].GetString();
		bool loginSuccess = rblx.logIn(cookie);
		if (!loginSuccess)
		{
			std::cout << "<=> Failed to log in to " << rblx.username << ", make sure your cookie is correct." << std::endl;
			continue;
		}

		std::cout << "<=> Successfully logged into account " << rblx.username << std::endl;
		// add cookie to token cache list
		addCookie(cookie);
		rblx.updateCurrency();
		std::cout << "<=> Current Balance: " << rblx.robuxAmount << " R$" << std::endl;
		rblx_sessions.push_back(rblx);
		successful_logins++;
	}

	std::cout << "<=> Logged into " << successful_logins << "/" << settings["accounts"].Size() << " accounts" << std::endl;

	sleepcp(2500);
	for (size_t i = 0; i < assetIds.size(); ++i)
	{
		if (currentAssetNum < idsPerThread)
		{
			passAssetIds.push_back(assetIds[i]);
			++currentAssetNum;
		}
		else
		{
			// add the remaining id(s)
			passAssetIds.push_back(assetIds[i]);

			for (int b = 0; b < settings["Amplify"].GetInt(); ++b)
			{
				std::cout << "<=> Starting thread " << threadNum << std::endl;
				try
				{
					sleepcp(500);
					CSniperThread sniperthread(rblx_sessions, passAssetIds, threadNum);
					std::thread th(&CSniperThread::run, sniperthread);
					th.detach();
					++threadNum;
				}
				catch (const std::exception & e)
				{
					std::cout << "<=> FAILURE while trying to start thread " << threadNum << std::endl;
					std::clog << e.what() << std::endl;
					++threadNum;
					sleepcp(8000);
				}
			}
			// reset
			passAssetIds.clear();
			currentAssetNum = 1;
		}
	}

	// start checking whitelist
	int failedChecks = 0;
	while (true) {
		sleepcp(10000000);

		// check if there are more than 4 failed checks
		if (failedChecks > 4) bWhitelistValidated = false;

	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
