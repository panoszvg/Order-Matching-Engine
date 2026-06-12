#include "FixField.h"

FixField::FixField(const string& value) : field(value) {}

string FixField::getValue() const {
	return field;
}
