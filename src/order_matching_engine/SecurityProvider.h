#pragma once

#include "ISecurityProvider.h"
#include <string>

using std::string;

class SecurityProvider : public ISecurityProvider {
private:
	string path;

public:
	explicit SecurityProvider(string path) : path(std::move(path)) {}
	std::unordered_map<string, std::unique_ptr<Security>> loadSecurities() override;

};
