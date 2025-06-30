#ifndef SECURITY_H
#define SECURITY_H

#include <string>
using std::string;

class Security {
public:
    Security(string symbol, double tickSize, double bucketSize);

    const string& getSymbol() const;
    double getTickSize() const;
    double getBucketSize() const;

private:
    string symbol;
    double tickSize;
    double bucketSize;
};

#endif // SECURITY_H
