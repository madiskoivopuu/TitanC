#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <iterator>

#pragma once

#ifndef _MAINFUNCS_H_
#define _MAINFUNCS_H_
#define min_(a,b) a < b ? a : b


struct CaseInsensitive {
	bool operator()(std::string const& left, std::string const& right) const {
		size_t const size = min_(left.size(), right.size());

		for (size_t i = 0; i != size; ++i) {
			char const lowerLeft = std::tolower(left[i]);
			char const lowerRight = std::tolower(right[i]);

			if (lowerLeft < lowerRight) { return true; }
			if (lowerLeft > lowerRight) { return false; }

			// if equal? continue!
		}

		// same prefix? then we compare the length
		return left.size() < right.size();
	}
};

extern rapidjson::Document settings;
extern bool bWhitelistValidated;
extern std::vector<std::string> cookies;
extern std::vector<std::string> pricecheckCookies;
extern std::vector<std::string> proxies;
extern std::map<std::string, std::string> cachedTokens;

extern void eraseSubStr(std::string& mainStr, const std::string& toErase);
extern void sleepcp(int milliseconds);

extern std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter);
extern std::map<std::string, std::string, CaseInsensitive> parse_headers(std::string str);

extern void addCookie(std::string cookie);
extern bool validateWhitelist();
extern bool validateVersion();
extern bool validateSettings();
extern bool isStringNumber(std::string s);

#endif // !_MAINFUNCS_H_
