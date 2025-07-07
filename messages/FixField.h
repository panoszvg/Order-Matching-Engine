#ifndef IFIXFIELD_H
#define IFIXFIELD_H

#include <string>

using std::string;

enum FIX_TYPES { FIX_STRING, FIX_CHAR, FIX_INT };

class FixField {
private:
	string field;
	FIX_TYPES type;

public:
	FixField(const string& _field, const int _type);

	string getValue();
};

#endif // IFIXFIELD_H
