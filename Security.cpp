#include <string>
using namespace std;

class Security {
private:
	string name;
	double tick_size;

public:
	Security(string name, double tick_size) {
		this->name = name;
		this->tick_size = tick_size;
	}

	string getName() {
		return this->name;
	}

	double getTickSize() {
		return this->tick_size;
	}

};