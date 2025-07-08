#include "Parser.h"

Parser::Parser(PARSER_MODE parserMode) : parserMode(parserMode) {}

shared_ptr<IMessage> Parser::parse(const string& message) {
	shared_ptr<IMessage> newMessage;

	switch (parserMode) {
	case FIX_MESSAGE:
		newMessage = make_shared<FixMessage>();
		break;
	case SIMPLE_MESSAGE:
	default:
		newMessage = make_shared<SimpleMessage>();
		break;
	}

	newMessage->populate(message);
	return newMessage;
}



