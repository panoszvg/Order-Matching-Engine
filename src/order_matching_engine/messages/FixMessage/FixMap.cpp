#include "FixMap.h"

void FixMap::addField(const int fieldTag, const string& input) {
	fields.emplace(fieldTag, FixField(input));
}

const map<int, FixField>& FixMap::getFields() const {
	return fields;
}
