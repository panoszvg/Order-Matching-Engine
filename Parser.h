#ifndef PARSER_H
#define PARSER_H

#pragma once

#include "Order.h"
#include "messages/FixMessage/FixMessage.h"
#include "messages/SimpleMessage/SimpleMessage.h"
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>

using std::string;
using std::shared_ptr;
using std::make_shared;

enum PARSER_MODE { SIMPLE_MESSAGE, FIX_MESSAGE };

class Parser {
private:
    PARSER_MODE parserMode;

public:
    Parser(PARSER_MODE parserMode = SIMPLE_MESSAGE);
    shared_ptr<IMessage> parse(const string& message);

};

#endif // PARSER_H
