#pragma once
#include <string>
#include <vector>
#include <utility>

namespace i18n
{
	void setLanguage(const std::string& lang);
	const std::string& getLanguage();
	const char* tr(const char* key);
	inline std::string tr(const std::string& key) { return std::string(tr(key.c_str())); }
	const std::vector<std::pair<std::string, std::string>>& getAvailableLanguages();
	bool loadLanguageFromFile(const std::string& path);
}

#define _(x) i18n::tr(x)