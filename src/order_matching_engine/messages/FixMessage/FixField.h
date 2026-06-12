#ifndef IFIXFIELD_H
#define IFIXFIELD_H

#include <string>

using std::string;

class FixField {
private:
	string field;

public:
	explicit FixField(const string& value);

	string getValue() const;
};

#endif // IFIXFIELD_H
