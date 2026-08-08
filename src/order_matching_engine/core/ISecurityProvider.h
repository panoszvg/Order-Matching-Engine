#ifndef ISECURITYPROVIDER_H
#define ISECURITYPROVIDER_H

#pragma once

#include "Security.h"
#include <vector>
#include <memory>
#include <unordered_map>

struct ISecurityProvider {
	virtual std::unordered_map<string, std::unique_ptr<Security>> loadSecurities() = 0;
	virtual ~ISecurityProvider() = default;
};

#endif // ISECURITYPROVIDER_H