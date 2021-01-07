#ifndef _CSNIPERTHREAD_H_
#define _CSNIPERTHREAD_H_

#include "rblx.h"
#include <curl/curl.h>
#include <vector>
#include <string>
#include <map>

class CSniperThread {
public:
	CSniperThread(std::vector<Roblox> rblx_sessions, std::vector<std::string> ids, int threadnum);
	double percentage(int percent, int64_t whole);
	bool isSnipeProfitable(int64_t price1, int64_t price2, double profit, double profitPercent);
	bool shouldCycleAccount();
	void trySnipeAsset(std::string assetId, int64_t productId, int64_t snipePrice, int64_t UserAssetID, int64_t SellerID, double profitMargin, int64_t Old_Price);
	void run();

private:
	//void AssetBlocker = 0;
	CURL* curl;
	Roblox rblx;
	int threadNum;
	std::string fullWebhookUrl;
	std::vector<std::string> assetIds;
	std::vector<Roblox> rblx_sessions;
	std::vector<curl_t> curls;
	std::map<std::string, int64_t> productIds;
	std::map<std::string, int64_t> cachedPrices;
};

#endif // !_CSNIPERTHREAD_H_

