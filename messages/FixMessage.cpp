#include "FixMessage.h"
#include <string>
#include <sstream>

FixMessage::FixMessage() {
	header = std::make_shared<FixMap>();
	body   = std::make_shared<FixMap>();
	trailer= std::make_shared<FixMap>();
}

void FixMessage::populate(const string& message) {
	std::stringstream ss(message);
	string token;
	auto& currentMap = header;

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos != string::npos) {
			int tag = stoi(token.substr(0, equalPos));
			string value = token.substr(equalPos + 1);

			if (tag == 10) {
				currentMap = trailer;
			}

			currentMap->addField(tag, value);

			if (tag == 52) {
				currentMap = body;
			}
		}
	}

}

void FixMessage::isValid() {}

string FixMessage::getValue(int tag) {
	auto fields = header->getFields();
	if (fields.find(tag) != fields.end()) {
		return fields.at(tag)->getValue();
	}

	fields = body->getFields();
	if (fields.find(tag) != fields.end()) {
		return fields.at(tag)->getValue();
	}

	fields = trailer->getFields();
	if (fields.find(tag) != fields.end()) {
		return fields.at(tag)->getValue();
	}

	return "";
}

void FixMessage::print() {
	auto& curr = header;
	for (auto [tag, field] : curr->getFields()) {
		logger->info("Tag {} = {}", tag, field->getValue());
	}

	curr = body;
	for (auto [tag, field] : curr->getFields()) {
		logger->info("Tag {} = {}", tag, field->getValue());
	}

	curr = trailer;
	for (auto [tag, field] : curr->getFields()) {
		logger->info("Tag {} = {}", tag, field->getValue());
	}
}


