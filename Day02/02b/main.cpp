#include <array>
#include <fstream>
#include <iostream>
#include <string>

int main() {
	std::ifstream input("input.txt");
	
	if(!input.is_open()) {
		std::cerr << "Could not open the input file.\n";
		exit(1);
	}

	std::string line{};

	std::array<std::array<int, 3>, 3> table = { 
		{
			{3, 4, 8},
			{1, 5, 9},
			{2, 6, 7}
		}
	};

	int score = 0;

	while(std::getline(input, line)) {
		[[unlikely]]
		if(line.length() != 3) {
			std::cerr << "Invalid input! (Line of length " << line.length() << '\n';
			exit(2);
		}

		char const enemy = line.front();
		char const outcome = line.back();

		int const enemyIdx = int(enemy - 'A');
		int const outcomeIdx = int(outcome - 'X');

		[[unlikely]]
		if(enemyIdx < 0 || enemyIdx >= table.size()) {
			std::cerr << "Invalid enemy index " << enemyIdx << '\n';
			exit(3);
		}

		[[unlikely]]
		if(outcomeIdx < 0 || outcomeIdx >= table.front().size()) {
			std::cerr << "Invalid outcome index " << outcomeIdx << '\n';
			exit(4);
		}

		score += table[enemyIdx][outcomeIdx];
	}

	input.close();

	std::cout << "Score: " << score << '\n';

}