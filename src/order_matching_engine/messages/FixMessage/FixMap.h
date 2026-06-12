#ifndef IFIXMAP_H
#define IFIXMAP_H

#pragma once

#include "FixField.h"
#include <map>
#include <string>

using std::map;
using std::string;

class FixMap {
private:
	map<int, FixField> fields;

public:
	void addField(const int fieldTag, const string& input);
	const map<int, FixField>& getFields() const;

};

#endif // IFIXMAP_H
