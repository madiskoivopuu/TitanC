#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifndef _RBLX_H_
#define _RBLX_H_

struct curl_t {
	CURL* curl;
	std::string proxy;
	bool priceCheckFailed;
};

class Roblox {
public:
	int64_t robuxAmount = 0;
	std::string username;
	std::string cookie;

	Roblox();
	std::string getItemThumbnail(std::string itemid);
	std::map<std::string, int64_t> cacheProductIds(std::vector<std::string> ids);

	int64_t getProductId(std::string assetId);
	bool getProductInfo(std::string assetId, rapidjson::Document& jsonData);
	bool getUserDetails(std::string url);
	int64_t updateCurrency();
	bool sellItem(std::string assetId, int64_t UserAssetID, int64_t price);
	bool buyItem(int64_t productId, int64_t price, int64_t UserAssetID, int64_t SellerID, rapidjson::Document& jsonData);
	bool getSellers(std::string assetIds, std::string cookie, std::vector<int64_t>& prices, std::vector<int64_t>& UserAssetIDs, std::vector<int64_t>& SellerIDs);
	bool getPricesFromCatalog(std::string& url, std::string cookie, std::map<std::string, int64_t>& data);
	bool logIn(std::string cookie);
private:
	CURL* curl;
	bool isAccountUnder13 = false;
};

#endif 
