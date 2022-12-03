#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

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

	std::unordered_map<char, int> backpackCount;

	if(!input.is_open()) {
		std::cerr << "Cannot open input file.\n";
		return 1;
	}

	std::string line{};

	int totalPriority{ 0 };

	int lineIdx = 0;

	while(std::getline(input, line)) {
		++lineIdx;
		
		if(lineIdx == 1) {
			for(auto const c : line) {
				backpackCount[c] = 1;
			}
		}

		else if(lineIdx == 2) {
			for(auto const c : line) {
				auto& previousLine = backpackCount[c];

				if(previousLine == 1) {
					previousLine = 2;
				}
			}
		}
		else if(lineIdx == 3) {
			lineIdx = 0;
			
			for(auto const c : line) {
				auto& previousLine = backpackCount[c];

				if(previousLine == 2) {
					previousLine = 3;
				}
			}

			for(auto const [c, count] : backpackCount) {
				if(count == 3) {
					totalPriority += getPriority(c);
				}
			}

			backpackCount.clear();
		}

	}

	std::cout << "Sum of priorities: " << totalPriority << '\n';

	return 0;
}