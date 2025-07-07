#ifndef PARSER_H
#define PARSER_H

#pragma once

#include "Order.h"
#include "messages/FixMessage.h"
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>

using std::string;
using std::shared_ptr;
using std::make_shared;

class Parser {
public:
    Parser() = default;

    shared_ptr<Order> parse(const string& message);

};

#endif // PARSER_H
