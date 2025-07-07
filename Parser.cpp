#include "Parser.h"

shared_ptr<Order> Parser::parse(const string& message) {
    shared_ptr<FixMessage> newMessage = make_shared<FixMessage>();
    newMessage->populate(message);
    string  tag_55 = newMessage->getValue(55);
    int     tag_54 = stoi(newMessage->getValue(54));
    double  tag_38 = stod(newMessage->getValue(38));
    double  tag_44 = stod(newMessage->getValue(44));

    auto newOrder = make_shared<Order>(tag_55, (tag_54 == 1) ? BUY : SELL, tag_38, tag_44);
    return newOrder;
}



