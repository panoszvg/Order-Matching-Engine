#include "Parser.h"

shared_ptr<Order> Parser::parse(const string& message) {
    std::unordered_map<int, string> fields;
    std::stringstream ss(message);
    string token;

    while (std::getline(ss, token, SOH)) {
        auto equalPos = token.find('=');
        if (equalPos != string::npos) {
            int tag = stoi(token.substr(0, equalPos));
            string value = token.substr(equalPos + 1);
            fields[tag] = value;
        }
    }

    auto newOrder = make_shared<Order>(fields[55], (stoi(fields[54]) == 1) ? BUY : SELL, stod(fields[38]), stod(fields[44]));
    return newOrder;
}



