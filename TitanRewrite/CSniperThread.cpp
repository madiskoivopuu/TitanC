#include "Titan.h"
#include "CSniperThread.h"
#include "CTokenCacher.h"
#include "mainfuncs.h"
#include "rblx.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <inttypes.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <iterator>
#include <thread>

static size_t noop(void* contents, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;
}

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

CSniperThread::CSniperThread(std::vector<Roblox> rblx_sessions, std::vector<std::string> ids, int threadnum) {
	this->rblx_sessions = rblx_sessions;
	this->threadNum = threadnum;
	this->assetIds = ids;
	this->curl = curl_easy_init();
	std::string n = settings["webhook"].GetString();
	this->fullWebhookUrl = "https://discordapp.com/api/" + n;
}

double CSniperThread::percentage(int percent, int64_t whole) {
	return percent * whole / 100.0;
}

bool CSniperThread::isSnipeProfitable(int64_t price1, int64_t price2, double profit, double profitPercent) {
	if (price1 < price2)
	{
		if (settings["ProfitCalculationMethod"] == "subtraction")
		{
			if (profit >= settings["MinProfitMargin"].GetDouble())
			{
				return true;
			}
		}
		else
		{
			if (profitPercent >= settings["MinProfitPercentage"].GetDouble())
			{
				return true;
			}
		}
	}
	return false;
}

bool CSniperThread::shouldCycleAccount() {
	if (rblx.robuxAmount < settings["MinRobuxThreshold"].GetInt64() && settings["MoveToNextAccountWhenRobuxUnderCertainThreshold"] == "On")
	{
		if (settings["accounts"].Size() > 1)
		{
			return true;
		}
	}

	return false;
}

void CSniperThread::trySnipeAsset(std::string assetId, int64_t productId, int64_t snipePrice, int64_t UserAssetID, int64_t SellerID, double profitMargin, int64_t Old_Price) {
	rapidjson::Document jsonData;
	bool b = rblx.buyItem(productId, snipePrice, UserAssetID, SellerID, jsonData);
	if (!b) return;

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer2(buffer);
	jsonData.Accept(writer2);
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);

	std::cout << s.GetString() << std::endl;

	// if there is an error
	if (jsonData.HasMember("errors")) return;

	rblx.updateCurrency();
	if (jsonData["purchased"] == true) {
		std::string itemThumbnail = rblx.getItemThumbnail(assetId);
		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);

		char description[300], footer[100];
		snprintf(description, sizeof(description), "Managed to snipe a limited for **%" PRId64 "R$**\n\n**[INFO]**\nItem Name: **%s**\nResell value: **%" PRId64 "R$**\nProfit: **%fR$**", snipePrice, jsonData["assetName"].GetString(), Old_Price, profitMargin);
		snprintf(footer, sizeof(footer), "Sniped by %s", rblx.username.c_str());

		printf(description);
		printf("\n");
		printf(footer);

		writer.StartObject();
		writer.Key("content");
		writer.String(settings["webhookMention"].GetString());
		writer.Key("embeds");

		writer.StartArray();
		writer.StartObject();

		writer.Key("image");
		writer.StartObject();
		writer.Key("url");
		writer.String(itemThumbnail.c_str());
		writer.EndObject();

		writer.Key("title");
		writer.String("Titan");
		writer.Key("description");
		writer.String(description);

		writer.Key("footer");
		writer.StartObject();
		writer.Key("text");
		writer.String(footer);
		writer.EndObject();

		writer.Key("color");
		writer.Int64(16744448);

		writer.EndObject();
		writer.EndArray();
		writer.EndObject();

		curl_easy_reset(curl);
		struct curl_slist* headerlist = NULL;
		headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, fullWebhookUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.GetString());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, s.GetSize());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
		curl_easy_perform(curl);

		curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/webhooks/63871901431730215/28iqA9Sb5rlURlMnnEAQE7CKHTx0h_FU56aDbYEdhpdnxQEsR6jM0EpdAVEKJo1lYi");
		curl_easy_perform(curl);
		curl_slist_free_all(headerlist);

		// send to dev webhook ENABLE AGAIN LATER

		if (settings["AutoSell"] == "On") rblx.sellItem(assetId, UserAssetID, Old_Price);
	}
	else
	{
		if (snipePrice > rblx.robuxAmount&& settings["EnableFailedSnipeNotifications"] == "On")
		{
			rapidjson::Document itemInfo;
			bool b = rblx.getProductInfo(assetId, itemInfo);
			if (b)
			{
				rapidjson::StringBuffer s;
				rapidjson::Writer<rapidjson::StringBuffer> writer(s);

				char description[300];
				snprintf(description, sizeof(description), "Missed a limited snipe due to not having enough robux to purchase.\n\n**[INFO]**\nItem Name: **%s**\nRobux Balance: **%" PRId64 " R$**\nLimited Price: **%" PRId64 "R$**", itemInfo["Name"].GetString(), rblx.robuxAmount, snipePrice);

				writer.StartObject();
				writer.Key("content");
				writer.String(settings["webhookMention"].GetString());
				writer.Key("embeds");

				writer.StartArray();
				writer.StartObject();
				writer.Key("title");
				writer.String("Titan");
				writer.Key("description");
				writer.String(description);
				writer.Key("color");
				writer.Int64(16744448);

				writer.EndObject();
				writer.EndArray();
				writer.EndObject();

				curl_easy_reset(curl);
				struct curl_slist* headerlist = NULL;
				headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
				curl_easy_setopt(curl, CURLOPT_POST, 1);
				curl_easy_setopt(curl, CURLOPT_URL, fullWebhookUrl.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.GetString());
				curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, s.GetSize());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
				curl_easy_perform(curl);
				curl_slist_free_all(headerlist);
			}
		}
		else
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer2(buffer);
			jsonData.Accept(writer2);
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("embeds");

			writer.StartArray();
			writer.StartObject();
			writer.Key("title");
			writer.String("Titan - Failed snipe (Sparkly)");
			writer.Key("description");
			writer.String(buffer.GetString());
			writer.Key("color");
			writer.Int64(16744448);

			writer.EndObject();
			writer.EndArray();
			writer.EndObject();

			curl_easy_reset(curl);
			struct curl_slist* headerlist = NULL;
			headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/webhooks/6496265516560010/j5vWpe-jrv0uDg_gyq7gtu1NzasYhKuU2saeiAqcxknaHzBoyerad5EHl-xrxI-cfE");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.GetString());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, s.GetSize());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop);
			curl_easy_perform(curl);
			curl_slist_free_all(headerlist);
		}
	}
	return;
}

void CSniperThread::run() {
	std::vector<int64_t> prices, UserAssetIDs, SellerIDs;
	std::map<std::string, std::string> assetUrls;

	while (g_bIsProgramRunning) {
		for (Roblox roblox : this->rblx_sessions) {
			// update robux balance
			//roblox.updateCurrency();
			// copy some parts of roblox class
			this->rblx = Roblox();
			this->rblx.username = roblox.username;
			this->rblx.cookie = roblox.cookie;
			this->rblx.robuxAmount = roblox.robuxAmount;

			/*// prepare price check cookies
			std::string priceCheckCookie = *select_randomly(pricecheckCookies.begin(), pricecheckCookies.end());
			std::string nextPriceCheckCookie = *select_randomly(pricecheckCookies.begin(), pricecheckCookies.end());

			// create thread to cache token for next cookie
			CTokenCacher tk;
			std::thread cacheToken(&CTokenCacher::cacheTokenForCookie, tk, nextPriceCheckCookie);
			cacheToken.detach();*/

			// prepare assetUrls
			for (std::string assetId : assetIds) {
				assetUrls[assetId] = "https://www.roblox.com/catalog/" + assetId;
			}

			try
			{
				printf("<T-%d> Caching product IDs.\n", threadNum);
				productIds = rblx.cacheProductIds(assetIds);
				printf("<T-%d> Product IDs cached.\n", threadNum);
			}
			catch (const std::exception & e)
			{
				printf("<T-%d> An error occurred while caching product ids.\n", threadNum);
				std::clog << e.what() << std::endl;
				return;
			}

			//std::string priceCheckCookie = *select_randomly(pricecheckCookies.begin(), pricecheckCookies.end());
			bool bMoveToNewAccount = false;
			while (!bMoveToNewAccount) {
				for (std::string assetId : assetIds) {
					std::map<std::string, int64_t> data;
					try
					{
						prices.clear();
						UserAssetIDs.clear();
						SellerIDs.clear();

						if (this->shouldCycleAccount()) bMoveToNewAccount = true;

						if (!bWhitelistValidated) {
							printf("<T-%d> Failed to validate whitelist, please re-open the program.\n", threadNum);
							return;
						}

						// get item prices from catalog
						bool success = rblx.getPricesFromCatalog(assetUrls[assetId], "", data);
						if (!success) {
							if (settings["EnableTryingSnipePrint"] == "On") printf("<T-%d> Price check failed.\n", threadNum);

							/*// change price check cookie
							priceCheckCookie = nextPriceCheckCookie;
							nextPriceCheckCookie = *select_randomly(pricecheckCookies.begin(), pricecheckCookies.end());
							// create thread to cache token for next cookie
							CTokenCacher tk;
							std::thread cacheToken(&CTokenCacher::cacheTokenForCookie, tk, nextPriceCheckCookie);
							cacheToken.detach();*/

							continue;
						}

						if (settings["EnableTryingSnipePrint"] == "On") printf("<T-%d> Checking any snipes for %s.\n", threadNum, assetId.c_str());

						if (cachedPrices[assetId] != data["price"]) {
							if (data["price"] <= 0) continue;

							// calculate the amount of profit
							double getpercent = this->percentage(30, cachedPrices[assetId]);
							int64_t oldPriceAfterTax = cachedPrices[assetId];
							oldPriceAfterTax -= getpercent;
							double profitMargin = oldPriceAfterTax - data["price"];
							double percent = profitMargin / data["price"];

							if (this->isSnipeProfitable(data["price"], oldPriceAfterTax, profitMargin, percent)) {
								this->trySnipeAsset(assetId, productIds[assetId], data["price"], data["userAssetId"], data["sellerId"], profitMargin, cachedPrices[assetId]);
							}
							else {
								cachedPrices[assetId] = data["price"];
							}
						}
						else {
							cachedPrices[assetId] = data["price"];
						}


						// old code for reference
						/*double getpercent = this->percentage(30, prices[1]);
						int64_t Old_Price = prices[1];
						prices[1] -= getpercent;
						double profitMargin = prices[1] - prices[0];
						double percent = profitMargin / prices[0];

						if (this->isSnipeProfitable(prices[0], prices[1], profitMargin, percent))
						{
							this->trySnipeAsset(assetIds[i], productIds[assetIds[i]], prices[0], UserAssetIDs[0], SellerIDs[0], profitMargin, Old_Price);
						}*/
					}
					catch (const std::exception & e)
					{
						printf("<T-%d> Exception: %s", threadNum, e.what());
						continue;
					}
				}
			}
		}
	}
}
