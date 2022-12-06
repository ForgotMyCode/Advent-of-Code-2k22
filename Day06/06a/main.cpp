#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

int main() {
	std::fstream input("input.txt");

	[[unlikely]]
	if(!input.is_open()) {
		std::cerr << "Could not open input file.\n";
		exit(1);
	}

	std::string signal{};

	[[unlikely]]
	if(!std::getline(input, signal)) {
		std::cerr << "Could not read the signal\n";
		exit(2);
	}

	std::unordered_map<char, int> timeStamps{};

	int blockUntil = 4;

	for(int i = 0; i < signal.size(); ++i) {
		auto& timeStamp = timeStamps[signal[i]];
		
		blockUntil = std::max(blockUntil, timeStamp + 4);

		std::cout << "blockUntil = " << blockUntil << '\n';

		timeStamp = i;

		if(i >= blockUntil) {
			std::cout << "Solution 1: " << (i + 1) << std::endl;
			break;
		}
	}

	input.close();
}