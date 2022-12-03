#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

int getPriority(char c) {

	if(!std::isalpha(c)) {
		std::cerr << "Given character is not a letter\n";
		exit(2);
	}

	int priority{ 0 };

	if(std::islower(c)) {
		return int(c - 'a') + 1;
	}

	if(std::isupper(c)) {
		return int(c - 'A') + 27;
	}
	
	std::cerr << "WTF cannot get priority ???\n";
	exit(3);
}

int main() {
	std::ifstream input("input.txt");

	std::unordered_set<char> backpack1, backpack2;

	if(!input.is_open()) {
		std::cerr << "Cannot open input file.\n";
		return 1;
	}

	auto getCommonElement = [&backpack1, &backpack2]() -> char {
		for(auto element : backpack1) {
			if(backpack2.count(element)) {
				return element;
			}
		}

		std::cerr << "The sets have no common element!";
		exit(4);
	};

	std::string line{};

	int totalPriority{ 0 };

	while(std::getline(input, line)) {
		
		int const lineHalfSize = int(line.length()) / 2;

		for(int i = 0; i < lineHalfSize; ++i) {
			backpack1.emplace(line[i]);
			backpack2.emplace(line[i + lineHalfSize]);
		}

		auto const commonElement = getCommonElement();

		int const priority = getPriority(commonElement);

		totalPriority += priority;

		backpack1.clear();
		backpack2.clear();

	}

	std::cout << "Sum of priorities: " << totalPriority << '\n';

	return 0;
}