#include "FixMap.h"

void FixMap::addField(const int fieldTag, const string& input) {
	auto newField = make_shared<FixField>(input, fieldTag);
	fields.emplace(fieldTag, newField);
}

map<int, shared_ptr<FixField>> FixMap::getFields() {
	return fields;
}