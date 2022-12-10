#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

void crash(std::string const& message) {
	std::cout << message << '\n';
	assert(false);
	exit(1);
}

int strToInt(std::string_view str) {
	int result{};

	[[likely]]
	if(std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int " + std::string(str.begin(), str.end()));

	return 0;
}

class CPUsim {

public:
	void AddInstructionDelayed(int steps, int value = 0) {

		for(int i = 0; i < steps; ++i) {
			RegisterXhistory.emplace_back(RegisterX);
		}

		RegisterX += value;
		Cycle += steps;
	}

	void AddInstruction(int steps, int value = 0) {

		for(int i = 0; i < steps - 1; ++i) {
			RegisterXhistory.emplace_back(RegisterX);
		}

		RegisterX += value;
		Cycle += steps;
		RegisterXhistory.emplace_back(RegisterX);
	}

	int GetRegisterValueAt(int cycle) {
		return RegisterXhistory[cycle];
	}

	int GetCurrentCycle() const {
		return Cycle;
	}

	int GetLastRegisterValue() const {
		return RegisterXhistory.empty() ? 1 : RegisterXhistory.back();
	}

private:

	int RegisterX{ 1 };
	int Cycle{ 0 };
	std::vector<int> RegisterXhistory{};
};

int main() {

	int const screenWidth = 40;
	int const screenHeight = 6;

	std::ifstream input("input.txt");

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open input file.");
	}

	CPUsim cpu1{}, cpu2{};

	std::string screen{};

	auto drawPixels = [&screen, &cpu2, screenWidth]() mutable -> void {
		int const screenIdx = int(screen.size());
		int const registerValue = cpu2.GetLastRegisterValue();

		char const pixel = std::abs((screenIdx % screenWidth)- registerValue) <= 1 ? '#' : '.';

		std::cout << "S " << screenIdx << " R " << registerValue << " -> " << pixel << '\n';

		screen.push_back(pixel);
	};

	std::string line{};

	while(std::getline(input, line)) {
		drawPixels();
		
		if(line == "noop") {
			std::cout << "NOOP\n";
			cpu1.AddInstructionDelayed(1);
			cpu2.AddInstruction(1);
		}
		else { // !(line == "noop")
			if(line.starts_with("addx")) {
				std::cout << "ADDX\n";
				int const value = strToInt(std::string_view(std::next(line.begin(), 5), line.end()));
				drawPixels();
				cpu1.AddInstructionDelayed(2, value);
				cpu2.AddInstruction(2, value);
			}
			else { // !(line.starts_with("addx")) && (line == "noop")
				crash("Could not parse instruction line " + line);
			}
		}

	}

	input.close();

	int result = 0;

	for(int i = 19; i < cpu1.GetCurrentCycle(); i += 40) {
		auto const registerValue = cpu1.GetRegisterValueAt(i);
		std::cout << i << " | " << registerValue << '\n';
		result += (i + 1) * cpu1.GetRegisterValueAt(i);
	}

	std::cout << "Result: " << result << std::endl;

	for(int y = 0; y < screenHeight; ++y) {
		std::cout << std::string_view(std::next(screen.begin(), y * screenWidth), std::next(screen.begin(), (y + 1) * screenWidth)) << '\n';
	}

	return 0;
}