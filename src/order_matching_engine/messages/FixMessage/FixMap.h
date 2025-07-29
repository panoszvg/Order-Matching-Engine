#ifndef IFIXMAP_H
#define IFIXMAP_H

#pragma once

#include "FixField.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

using std::map;
using std::vector;
using std::string;
using std::shared_ptr;
using std::make_shared;

class FixMap {
private:
	map<int, shared_ptr<FixField>> fields;

public:
	void addField(const int fieldTag, const string& input);
	map<int, shared_ptr<FixField>> getFields();

};

#endif // IFIXMAP_H
