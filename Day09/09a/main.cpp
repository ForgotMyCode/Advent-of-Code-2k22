#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_set>

void crash(std::string const& message) {
	std::cerr << message << '\n';
	assert(false);
	exit(1);
}

std::pair<int, int> parseDirection(char direction) {
	switch(direction) {
	case 'U':
		return { 0, 1 };
	case 'D':
		return { 0,-1 };
	case 'L':
		return { -1, 0 };
	case 'R':
		return { 1, 0 };
	default:
		crash("Invalid direction " + direction);
	}

	return {};
}

int parseInt(std::string_view str) {
	
	int value{};

	[[likely]]
	if(std::from_chars(str.data(), str.data() + str.length(), value).ec == std::errc{}) {
		return value;
	}

	crash("Could not parse int " + std::string(str.begin(), str.end()));
	return 0;
}

int getDirection(int from, int to) {
	if(from == to) {
		return 0;
	}

	return to < from ? -1 : 1;
}

struct Coords {
	int x, y;

	bool isTouching(Coords const& other) const {
		return std::abs(x - other.x) <= 1 && std::abs(y - other.y) <= 1;
	}

	bool operator==(Coords const& other) const = default;
};

template<>
struct std::hash<Coords> {	
	std::size_t operator()(Coords const& coords) const noexcept {
		return std::hash<int>{}(coords.x) ^ std::hash<int>{}(coords.y);
	}
};

int main() {
	
	std::fstream input("input.txt");

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
	}

	Coords head{}, tail{};

	std::string line{};

	std::unordered_set<Coords> tailVisitedCoords{};
	tailVisitedCoords.insert(tail);

	while(std::getline(input, line)) {
		[[unlikely]]
		if(line.length() < 3) {
			crash("Could not parse line " + line);
		}

		auto const [dx, dy] = parseDirection(line[0]);

		int nSteps = parseInt(std::string_view(std::next(line.begin(), 2), line.end()));

		for(int step = 0; step < nSteps; ++step) {
			
			head.x += dx;
			head.y += dy;

			if(!tail.isTouching(head)) {

				int const tailDx = getDirection(tail.x, head.x);
				int const tailDy = getDirection(tail.y, head.y);

				tail.x += tailDx;
				tail.y += tailDy;
			
				tailVisitedCoords.insert(tail);

			}
		}
	}

	input.close();

	std::cout << "Tail visited coords size: " << tailVisitedCoords.size() << '\n';

	return 0;

}