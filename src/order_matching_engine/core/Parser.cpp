#include "Parser.h"

Parser::Parser(PARSER_MODE parserMode) : parserMode(parserMode) {}

std::unique_ptr<IMessage> Parser::parse(const string& message) {
	std::unique_ptr<IMessage> newMessage;

	switch (parserMode) {
	case FIX_MESSAGE:
		newMessage = std::make_unique<FixMessage>();
		break;
	case SIMPLE_MESSAGE:
	default:
		newMessage = std::make_unique<SimpleMessage>();
		break;
	}

	newMessage->populate(message);
	return newMessage;
}



