#ifndef SECURITY_H
#define SECURITY_H

#include <string>
using std::string;

class Security {
public:
	Security(string symbol, double tickSize, double lotSize);

	const string& getSymbol() const;
	double getTickSize() const;
	double getLotSize() const;

private:
	string symbol;
	double tickSize;
	double lotSize;
};

#endif // SECURITY_H
