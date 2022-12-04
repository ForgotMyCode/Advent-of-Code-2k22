#include <fstream>	
#include <format>
#include <iostream>
#include <ranges>
#include <string>
#include <tuple>

int main() {
	std::ifstream input("input.txt");

	if(!input.is_open()) {
		std::cerr << "Could not open the input file.\n";
		exit(1);
	}
	
	auto isContainedIn = [](int outerLeft, int outerRight, int innerLeft, int innerRight) -> bool {
		return outerLeft <= innerLeft && outerRight >= innerRight;
	};

	auto isEitherContainedIn = [&isContainedIn](int outerLeft, int outerRight, int innerLeft, int innerRight) -> bool {
		return isContainedIn(outerLeft, outerRight, innerLeft, innerRight) ||
			isContainedIn(innerLeft, innerRight, outerLeft, outerRight);
	};

	auto isIntervalOverlap = [](int aLeft, int aRight, int bLeft, int bRight) -> bool {
		return aLeft <= bRight && aRight >= bLeft;
	};

	int fullyContained{ 0 };
	int overlaps{ 0 };

	std::string line{};

	while(std::getline(input, line)) {
		
		auto elfPair = line | std::views::split(',') | std::views::transform(
			[](auto&& range) {
			return range | std::views::split('-') | std::views::transform(
				[](auto&& range) -> int {
					char* end{};
					return std::strtol(&*range.begin(), &end, 0);
				}
			);
		}
		) | std::views::transform(
			[](auto&& range) -> std::pair<int, int> {
				int const size = int(std::distance(range.begin(), range.end()));

				if(size != 2) {
					std::cerr << "Expected pair, got size " << size << '\n';
					exit(2);
				}

				return std::make_pair(*range.begin(), *std::next(range.begin()));
			}
		);

		int const elfSize = int(std::distance(elfPair.begin(), elfPair.end()));

		if(elfSize != 2) {
			std::cerr << "Expected elf pair, got size " << elfSize << '\n';
			exit(3);
		}

		auto const firstElf = *elfPair.begin(), secondElf = *std::next(elfPair.begin());

		auto [firstElfStart, firstElfEnd] = firstElf;
		auto [secondElfStart, secondElfEnd] = secondElf;

		if(isEitherContainedIn(firstElfStart, firstElfEnd, secondElfStart, secondElfEnd)) {
			++fullyContained;
			std::cout << '!';
		}

		if(isIntervalOverlap(firstElfStart, firstElfEnd, secondElfStart, secondElfEnd)) {
			++overlaps;
			std::cout << '*';
		}
		
		std::cout << std::format("{}-{},{}-{}\n", firstElfStart, firstElfEnd, secondElfStart, secondElfEnd);

	}

	input.close();

	std::cout << "Fully contained elf pairs: " << fullyContained << '\n';
	std::cout << "Overlaps: " << overlaps << std::endl;

	return 0;
}