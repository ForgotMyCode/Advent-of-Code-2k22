#include <charconv>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <ranges>

int strToInt(std::string const& str) {
	int result{};

	[[likely]]
	if(std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	std::cerr << "Could not convert string " << str << " to int.\n";
	exit(3);
}

int main() {
	std::fstream input("input.txt");

	if(!input.is_open()) {
		std::cerr << "Could not open the input file\n";
		exit(1);
	}

	std::string line{};

	std::vector<std::string> stackLines{};

	while(std::getline(input, line) && line.length() > 0) {
		stackLines.emplace_back(line);
	}

	int const n = ((int(stackLines.back().size()) - 3) / 4) + 1;

	stackLines.pop_back();

	std::vector<std::vector<char>> crateStacks(n);

	for(auto it = stackLines.crbegin(); it != stackLines.crend(); ++it) {
		for(int i = 0; i < n; ++i) {
			int const lineIdx = (4 * i) + 1;

			auto const crate = (*it)[lineIdx];

			if(crate == ' ') {
				continue;
			}

			crateStacks[i].emplace_back(crate);
		}
	}

	auto crateStacks2 = crateStacks;

	while(std::getline(input, line)) {
		
		auto tokens = line | std::views::split(' ');

		[[unlikely]]
		if(std::distance(tokens.begin(), tokens.end()) != 6) {
			std::cerr << "Could not parse 6 tokens, got " << std::distance(tokens.begin(), tokens.end()) << '\n';
			exit(2);
		}

		auto countIt = *std::next(tokens.begin(), 1);
		auto sourceIt = *std::next(tokens.begin(), 3);
		auto targetIt = *std::next(tokens.begin(), 5);
	
		std::string const countStr(countIt.begin(), countIt.end());
		std::string const sourceStr(sourceIt.begin(), sourceIt.end());
		std::string const targetStr(targetIt.begin(), targetIt.end());

		auto const count = strToInt(countStr);
		auto const source = strToInt(sourceStr);
		auto const target = strToInt(targetStr);

		auto& sourceStack = crateStacks[source - 1];
		auto& targetStack = crateStacks[target - 1];

		for(int i = 0; i < count; ++i) {

			auto const crate = sourceStack.back();
			sourceStack.pop_back();

			targetStack.push_back(crate);

		}
		
		auto& sourceStack2 = crateStacks2[source - 1];
		auto& targetStack2 = crateStacks2[target - 1];

		auto sourceStartIt = std::prev(sourceStack2.end(), count);
		targetStack2.insert(targetStack2.end(), sourceStartIt, sourceStack2.end());
		sourceStack2.erase(sourceStartIt, sourceStack2.end());
	}

	for(auto const& crates : crateStacks) {
		std::cout << crates.back();
	}

	std::cout << std::endl;

	for(auto const& crates : crateStacks2) {
		std::cout << crates.back();
	}

	std::cout << std::endl;

	return 0;

}