#include "Titan.h"
#include "CTokenCacher.h"
#include "rblx.h"
#include "mainfuncs.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>

CTokenCacher::CTokenCacher() {
	curl = curl_easy_init();
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static size_t noop(void* contents, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;
}

void CTokenCacher::cacheTokens() {
	while (g_bIsProgramRunning) {
		sleepcp(2500);

		for (auto cookie : cookies)
		{
			if (!g_bIsProgramRunning) return;

			curl_easy_reset(curl);
			CURLcode res;
			struct curl_slist* headerlist = NULL;
			std::string cookieHeader, headerData;
			cookieHeader = "Cookie: .ROBLOSECURITY=" + cookie;
			headerlist = curl_slist_append(headerlist, cookieHeader.c_str());
			headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
			headerlist = curl_slist_append(headerlist, "Content-Length: 0");

			try
			{
				curl_easy_setopt(curl, CURLOPT_URL, "https://catalog.roblox.com/v1/catalog/items/details"); //https://economy.roblox.com/v1/purchases/products/118805948
				curl_easy_setopt(curl, CURLOPT_POST, 1);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
				curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);
				res = curl_easy_perform(curl);
				curl_slist_free_all(headerlist);
				if (res == CURLE_OK) {
					std::map<std::string, std::string, CaseInsensitive> headerList = parse_headers(headerData);
					if (headerList.count("X-CSRF-TOKEN"))
					{
						cachedTokens[cookie] = headerList["X-CSRF-TOKEN"];
					}
					else
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}
			catch (const std::exception & e)
			{
				std::cout << e.what() << std::endl;
				continue;
			}
		}
	}
}

void CTokenCacher::cacheTokenForCookie(std::string cookie) {
	for(int _ = 0; _ < 3; _++) {
		curl_easy_reset(curl);
		CURLcode res;
		struct curl_slist* headerlist = NULL;
		std::string cookieHeader, headerData;
		cookieHeader = "Cookie: .ROBLOSECURITY=" + cookie;
		headerlist = curl_slist_append(headerlist, cookieHeader.c_str());
		headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
		headerlist = curl_slist_append(headerlist, "Content-Length: 0");

		try
		{
			curl_easy_setopt(curl, CURLOPT_URL, "https://catalog.roblox.com/v1/catalog/items/details");
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);
			res = curl_easy_perform(curl);
			curl_slist_free_all(headerlist);
			if (res == CURLE_OK) {
				std::map<std::string, std::string, CaseInsensitive> headerList = parse_headers(headerData);
				if (headerList.count("X-CSRF-TOKEN"))
				{
					cachedTokens[cookie] = headerList["X-CSRF-TOKEN"];
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
		catch (const std::exception & e)
		{
			std::cout << e.what() << std::endl;
			continue;
		}
	}
}