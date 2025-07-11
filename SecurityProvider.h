#pragma once

#include "ISecurityProvider.h"
#include <string>

using std::string;

class SecurityProvider : public ISecurityProvider {
private:
    string path;

public:
	explicit SecurityProvider(string path) : path(std::move(path)) {}
	std::vector<std::shared_ptr<Security>> loadSecurities() override;

};
