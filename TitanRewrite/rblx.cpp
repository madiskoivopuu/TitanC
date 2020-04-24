#include <cstddef>
#include <rapidjson/error/en.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "mainfuncs.h"
#include "rblx.h"

#include <iostream>
#include <sstream>
#include <string>
#include <istream>
#include <iterator>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <random>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static size_t WriteCallbackSnipe(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	std::string& str = *(std::string*)userp;
	if (str.find("data-current-time") != std::string::npos) {
		return 0;
	}
	return size * nmemb;
}

static size_t noop(void* contents, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;
}

bool b2b = false;

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

Roblox::Roblox() {
	curl = curl_easy_init();
}

int64_t Roblox::getProductId(std::string assetId) {
	curl_easy_reset(curl);
	CURLcode res;
	std::string response;
	std::string url = "https://api.roblox.com/marketplace/productinfo?assetId=" + assetId;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
	{
		rapidjson::Document jsonData;
		jsonData.Parse(response.c_str());
		if (jsonData.HasParseError())
		{
			return 0;
		}
		else if (!jsonData.HasMember("ProductId"))
		{
			return 0;
		}
		else
		{
			return jsonData["ProductId"].GetInt64();
		}

	}
	else
	{
		return 0;
	}
}

bool Roblox::getProductInfo(std::string assetId, rapidjson::Document& jsonData) {
	curl_easy_reset(curl);
	CURLcode res;
	std::string response;
	std::string url = "https://api.roblox.com/marketplace/productinfo?assetId=" + assetId;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
	{
		jsonData.Parse(response.c_str());
		if (jsonData.HasParseError())
		{
			return false;
		}
		else
		{
			return true;
		}

	}
	else
	{
		return false;
	}
}

std::map<std::string, int64_t> Roblox::cacheProductIds(std::vector<std::string> ids) {
	std::map<std::string, int64_t> cache;
	for (size_t i = 0; i < ids.size(); ++i)
	{
		cache[ids[i]] = this->getProductId(ids[i]);
	}
	return cache;
}

int64_t Roblox::updateCurrency() {
	curl_easy_reset(curl);
	CURLcode res;
	std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + this->cookie;
	std::string content;
	struct curl_slist* headerlist = NULL;
	headerlist = curl_slist_append(headerlist, cookieHeader.c_str());

	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_URL, "https://api.roblox.com/currency/balance");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	res = curl_easy_perform(curl);
	curl_slist_free_all(headerlist);
	if (res == CURLE_OK)
	{
		rapidjson::Document jsonData;
		jsonData.Parse(content.c_str());
		if (jsonData.HasParseError())
		{
			this->robuxAmount = -1;
			return this->robuxAmount;
		}
		else
		{
			this->robuxAmount = jsonData["robux"].GetInt64();
			return this->robuxAmount;
		}
	}
	else
	{
		this->robuxAmount = -1;
		return this->robuxAmount;
	}
}

bool Roblox::getUserDetails(std::string url) {
	if (isAccountUnder13)
	{
		eraseSubStr(url, "https://web.roblox.com/users/");
	}
	else
	{
		eraseSubStr(url, "https://www.roblox.com/users/");
	}
	eraseSubStr(url, "/profile");

	CURLcode res;
	long httpCode;
	std::string response;
	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, std::string("https://api.roblox.com/users/" + url).c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		if (httpCode == 200)
		{
			rapidjson::Document jsonData;
			jsonData.Parse(response.c_str());
			if (jsonData.HasParseError())
			{
				return false;
			}
			else
			{
				username = jsonData["Username"].GetString();
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool Roblox::getSellers(std::string assetId, std::string cookie, std::vector<int64_t>& prices, std::vector<int64_t>& UserAssetIDs, std::vector<int64_t>& SellerIDs) {
	curl_easy_reset(curl);

	CURLcode res;
	std::string content;
	rapidjson::Document sellers;
	char url[100];
	snprintf(url, sizeof(url), "https://economy.roblox.com/v1/assets/%s/resellers?limit=10", assetId.c_str());

	std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + cookie;
	struct curl_slist* headerlist = NULL;
	headerlist = curl_slist_append(headerlist, cookieHeader.c_str());

	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	res = curl_easy_perform(curl);
	curl_slist_free_all(headerlist);
	if (res == CURLE_OK)
	{
		sellers.Parse(content.c_str());
		if (sellers.HasParseError()) {
			return false;
		}

		if (sellers.HasMember("errors")) {
			return false;
		}

		for (rapidjson::Value::ConstValueIterator itr = sellers["data"].Begin(); itr != sellers["data"].End(); ++itr) {
			prices.push_back((*itr)["price"].GetInt64());
			UserAssetIDs.push_back((*itr)["userAssetId"].GetInt64());
			SellerIDs.push_back((*itr)["seller"]["id"].GetInt64());
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool Roblox::getPricesFromCatalog(std::string& url, std::string cookie, std::map<std::string, int64_t>& data) {
	// prepare variables
	long status_code;
	CURLcode res;
	std::string content;
	char* endurl;

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackSnipe);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	res = curl_easy_perform(curl);

	if (res == CURLE_WRITE_ERROR) {
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &endurl);
		if (strcmp(endurl, url.c_str()) != 0) url = std::string(endurl);

		// cut the content down for parser
		size_t pos1 = content.find("<div id=\"item-container\"");
		content = content.substr(pos1);
		size_t pos2 = content.find(">");
		content = content.substr(0, pos2 + 1);

		htmlParserCtxtPtr parser_context = htmlNewParserCtxt();
		htmlDocPtr document = htmlCtxtReadMemory(parser_context, content.c_str(), content.size(), NULL, NULL, HTML_PARSE_NOWARNING|HTML_PARSE_NOERROR);
		htmlFreeParserCtxt(parser_context);
		if (!document) return false;

		xmlXPathContextPtr context = xmlXPathNewContext(document);
		xmlXPathObjectPtr object = xmlXPathEval((xmlChar*)"//div[@id='item-container']", context);
		xmlXPathFreeContext(context);
		if (!object || !object->nodesetval->nodeTab) {
			xmlFreeDoc(document);
			return false;
		}

		data["price"] = std::atoll(reinterpret_cast<const char*>(xmlGetProp(object->nodesetval->nodeTab[0], (xmlChar*)"data-expected-price")));
		data["sellerId"] = std::atoll(reinterpret_cast<const char*>(xmlGetProp(object->nodesetval->nodeTab[0], (xmlChar*)"data-expected-seller-id")));
		data["userAssetId"] = std::atoll(reinterpret_cast<const char*>(xmlGetProp(object->nodesetval->nodeTab[0], (xmlChar*)"data-lowest-private-sale-userasset-id")));

		xmlXPathFreeObject(object);
		xmlFreeDoc(document);
		return true;
	}
	else {
		return false;
	}

}

std::string Roblox::getItemThumbnail(std::string itemid) {
	CURLcode res;
	std::string url = "https://www.roblox.com/asset-thumbnail/image?width=110&height=110&format=png&assetId=" + itemid;
	char* endurl;

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &endurl);
		std::string surl(endurl);
		return surl;
	}
	else
	{
		return "https://tr.rbxcdn.com/9529c224d359b5a1e1d0b1a8ec02d24e/110/110/Hat/Png";
	}
}

bool Roblox::sellItem(std::string assetId, int64_t UserAssetID, int64_t price) {
	CURLcode res;
	std::string content, headers;
	std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + this->cookie;
	std::string csrf = "X-CSRF-TOKEN: " + cachedTokens[this->cookie];
	struct curl_slist* headerlist = NULL, * headerlist2 = NULL;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	headerlist = curl_slist_append(headerlist, cookieHeader.c_str());
	headerlist = curl_slist_append(headerlist, csrf.c_str());

	std::string url = "https://www.roblox.com/asset/toggle-sale";
	char data[100];
	snprintf(data, sizeof(data), "assetId=%s&userAssetId=%" PRId64 "&price=%" PRId64 "&sell=true", assetId.c_str(), UserAssetID, price);

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
	res = curl_easy_perform(curl);
	curl_slist_free_all(headerlist);
	if (res == CURLE_OK) {
		std::map<std::string, std::string, CaseInsensitive> headerList = parse_headers(headers);
		if (headerList.count("X-CSRF-TOKEN"))
		{
			std::string csrf_token = "X-CSRF-TOKEN: " + headerList["X-CSRF-TOKEN"];
			headerlist2 = curl_slist_append(headerlist2, cookieHeader.c_str());
			headerlist2 = curl_slist_append(headerlist2, "Content-Type: application/json; charset=UTF-8");
			headerlist2 = curl_slist_append(headerlist2, csrf_token.c_str());
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist2);
			res = curl_easy_perform(curl);
			curl_slist_free_all(headerlist2);
			if (res == CURLE_OK) {
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool Roblox::buyItem(int64_t productId, int64_t price, int64_t UserAssetID, int64_t SellerID, rapidjson::Document& jsonData) {
	CURLcode res;
	std::string content, headers;
	std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + this->cookie;
	std::string csrf = "X-CSRF-TOKEN: " + cachedTokens[this->cookie];
	struct curl_slist* headerlist = NULL, * headerlist2 = NULL;
	headerlist = curl_slist_append(headerlist, cookieHeader.c_str());
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
	headerlist = curl_slist_append(headerlist, csrf.c_str());

	char url[200];
	snprintf(url, sizeof(url), "https://economy.roblox.com/v1/purchases/products/%" PRId64, productId);

	// build json
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);

	writer.StartObject();
	writer.Key("expectedCurrency");
	writer.Int64(1);
	writer.Key("expectedPrice");
	writer.Int64(price);
	writer.Key("expectedSellerId");
	writer.Int64(SellerID);
	writer.Key("userAssetId");
	writer.Int64(UserAssetID);
	writer.EndObject();

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.GetString());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, s.GetSize());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	res = curl_easy_perform(curl);
	curl_slist_free_all(headerlist);
	std::cout << "buy res : " << res << std::endl;
	std::cout << content << std::endl;
	if (res == CURLE_OK) {
		std::map<std::string, std::string, CaseInsensitive> headerList = parse_headers(headers);
		if (headerList.count("X-CSRF-TOKEN"))
		{
			std::string csrf_token = "X-CSRF-TOKEN: " + headerList["X-CSRF-TOKEN"];
			headerlist2 = curl_slist_append(headerlist2, cookieHeader.c_str());
			headerlist2 = curl_slist_append(headerlist2, "Content-Type: application/json; charset=UTF-8");
			headerlist2 = curl_slist_append(headerlist2, csrf_token.c_str());

			curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist2);
			res = curl_easy_perform(curl);
			curl_slist_free_all(headerlist2);
			if (res == CURLE_OK) {
				jsonData.Parse(content.c_str());
				if (jsonData.HasParseError())
				{
					return false;
				}
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			jsonData.Parse(content.c_str());
			if (jsonData.HasParseError())
			{
				return false;
			}
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool Roblox::logIn(std::string cookie) {
	// set cookie
	this->cookie = cookie;

	CURLcode res;
	long httpCode;
	struct curl_slist* headerlist = NULL;
	char* url;
	std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + cookie;
	headerlist = curl_slist_append(headerlist, cookieHeader.c_str());

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.roblox.com/users/profile");
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
		std::string surl(url);
		curl_easy_reset(curl);
		if (httpCode == 200)
		{
			if (surl.find("/profile") != std::string::npos)
			{
				if (surl.find("https://web.") != std::string::npos) isAccountUnder13 = true;

				return this->getUserDetails(surl);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		return false;
	}
	else
	{
		return false;
	}

}
