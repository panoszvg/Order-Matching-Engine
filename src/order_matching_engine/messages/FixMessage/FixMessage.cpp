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
			string tagStr = token.substr(0, equalPos);
			string value = token.substr(equalPos + 1);

			try {				
				int tag = std::stoi(tagStr);

				if (tag == 10) currentMap = trailer;
				currentMap->addField(tag, value);
				if (tag == 35) currentMap = body;

			} catch (const std::invalid_argument& e) {
				logger->error("Invalid tag in FIX field: '{}'. Skipping.", tagStr);
			} catch (const std::out_of_range& e) {
				logger->error("Out of range tag in FIX field: '{}'. Skipping.", tagStr);
			}
		}
	}

}

void FixMessage::isValid() {}

shared_ptr<Order> FixMessage::makeOrder() {
	try {
		string  tag_55 = this->getValue(55);
		int     tag_54 = stoi(this->getValue(54));
		double  tag_38 = stod(this->getValue(38));
		double  tag_44 = stod(this->getValue(44));

		shared_ptr<Order> newOrder = make_shared<Order>(tag_55, (tag_54 == 1) ? BUY : SELL, tag_38, tag_44);
		return newOrder;
	} catch (const std::exception& e) {
		logger->error("FIX makeOrder failed: {}", e.what());
		throw;
	}
}

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


