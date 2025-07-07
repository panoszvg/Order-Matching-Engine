#include "FixField.h"

FixField::FixField(const string& _field, const int _type): field(_field) {
	switch (_type) {
	case 0:
		type = FIX_STRING;
		break;
	case 1:
		type = FIX_CHAR;
		break;
	case 2:
		type = FIX_INT;
		break;
	default:
		break;
	}
};

string FixField::getValue() {
	return field;
}