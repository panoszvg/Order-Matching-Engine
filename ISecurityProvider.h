#ifndef ISECURITYPROVIDER_H
#define ISECURITYPROVIDER_H

#pragma once

#include "Security.h"
#include <vector>
#include <memory>

struct ISecurityProvider {
private:
	virtual std::vector<std::shared_ptr<Security>> loadSecurities() = 0;

public:
	virtual ~ISecurityProvider() = default;

};

#endif // ISECURITYPROVIDER_H