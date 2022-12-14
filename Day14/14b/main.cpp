#include <charconv>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

std::vector<std::string> readInputLines(std::string const& fileName) {
	std::ifstream input(fileName);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
	}

	std::vector<std::string> lines{};

	{
		std::string line;

		while(std::getline(input, line)) {
			lines.emplace_back(line);
		}
	}

	input.close();

	return lines;
}

int strToInt(std::string_view str) {
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int " + std::string(str));
	return 0;
}

struct Coord {
	int x, y;
};

std::vector<Coord> parseLine(std::string_view line) {

	auto coordsRange = line |
		std::views::split(std::string_view(" -> ")) |
		std::views::transform([](auto const& rawCoords) -> Coord {

			auto coordRange = rawCoords |
				std::views::split(',') |
				std::views::chunk(2) | // adjacent transform view not supported yet :(
				std::views::transform([](auto const& chunkRange) -> Coord {

				auto const& xRange = *chunkRange.begin();
				auto const& yRange = *std::next(chunkRange.begin());

				int const x = strToInt(std::string_view(xRange));
				int const y = strToInt(std::string_view(yRange));

				return Coord{ .x = x, .y = y };
			});

			return *coordRange.begin();

		});

	return std::vector<Coord>(coordsRange.begin(), coordsRange.end());

}

std::vector<std::vector<Coord>> parseInput(std::vector<std::string> const& rawLines) {

	std::vector<std::vector<Coord>> parsedCoords{};

	for(auto const& rawLine : rawLines) {
		parsedCoords.emplace_back(parseLine(rawLine));
	}

	return parsedCoords;
}

void findExtremes(std::vector<std::vector<Coord>> const& coords, int& minX, int& maxX, int& minY, int& maxY) {
	
	minX = std::numeric_limits<int>::max();
	maxX = std::numeric_limits<int>::min();

	minY = std::numeric_limits<int>::max();
	maxY = std::numeric_limits<int>::min();

	for(auto const& coordLine : coords) {
		for(auto const& coord : coordLine) {
			minX = std::min(minX, coord.x);
			maxX = std::max(maxX, coord.x);

			minY = std::min(minY, coord.y);
			maxY = std::max(maxY, coord.y);
		}
	}

}

enum class FieldType : char {
	Air = '.',
	Stone = '#',
	Sand = 'o',
	Spawner = '+'
};

int getDirection(int a, int b) {
	if(b > a) {
		return 1;
	}

	if(b < a) {
		return -1;
	}

	return 0;
}

class Cave {
public:
	Cave(int minX, int minY, int maxX, int maxY, Coord sandSource) : 
		MinX(minX), MaxX(maxX), MinY(minY), MaxY(maxY),
		Width(maxX + 1 - minX), Height(maxY + 1 - minY),
		CaveMap(Height, std::vector<FieldType>(Width, FieldType::Air)),
		SandSourceMapped(MapCoord(sandSource))
	{
		SandPath.push(SandSourceMapped);
		CaveMap[SandSourceMapped.y][SandSourceMapped.x] = FieldType::Spawner;
	}

	void AddStonePath(std::vector<Coord> const& coords) {		
		for(int i = 1; i < int(coords.size()); ++i) {
			AddStoneLine(MapCoord(coords[i]), MapCoord(coords[i - 1]));
		}
	}

	void PrintMap() const {
		std::cout << "Map:\n";
		for(auto const& row : CaveMap) {
			for(auto const& field : row) {
				std::cout << std::to_underlying(field);
			}
			std::cout << '\n';
		}
	}

	bool DropSand() {
		bool isSpaceFound;
		bool isOutOfMap;
		Coord nextCoord;

		do {			
			if(SandPath.empty()) {
				return false;
			}

			auto const& coord = SandPath.top();

			FindFreeSpaceUnder(coord, nextCoord, isSpaceFound, isOutOfMap);

			if(isOutOfMap) {
				return false;
			}

			if(!isSpaceFound) {
				CaveMap[coord.y][coord.x] = FieldType::Sand;
				SandPath.pop();
				return true;
			}

			// space found
			SandPath.push(nextCoord);
		} while(true);

	}

protected:
	void FindFreeSpaceUnder(Coord const& inCoord, Coord& outCoord, bool& isSpaceFound, bool& isOutOfMap) const {
		int const nextY = inCoord.y + 1;

		auto fellOutOfMap = [&isSpaceFound, &isOutOfMap]() mutable -> void {
			isSpaceFound = false;
			isOutOfMap = true;			
		};

		auto foundSpace = [&outCoord, &isSpaceFound, &isOutOfMap](Coord const& coord) mutable -> void {
			outCoord = coord;
			isSpaceFound = true;
			isOutOfMap = false;
		};

		if(nextY >= Height) {
			fellOutOfMap();
			return;
		}

		if(IsAirAt(inCoord.x, nextY)) {
			foundSpace(Coord{ .x = inCoord.x, .y = nextY });
			return;
		}

		int const leftX = inCoord.x - 1;

		if(leftX < 0) {
			fellOutOfMap();
			return;
		}

		if(IsAirAt(leftX, nextY)) {
			foundSpace(Coord{ .x = leftX, .y = nextY });
			return;
		}

		int const rightX = inCoord.x + 1;

		if(rightX >= Width) {
			fellOutOfMap();
			return;
		}

		if(IsAirAt(rightX, nextY)) {
			foundSpace(Coord{ .x = rightX, .y = nextY });
			return;
		}

		isSpaceFound = false;
		isOutOfMap = false;
		return;
	}

	bool IsAirAt(int x, int y) const {
		return CaveMap[y][x] == FieldType::Air;
	}

	void AddStoneLine(Coord const& a, Coord const& b) {
		int const dx = getDirection(a.x, b.x);
		int const dy = getDirection(a.y, b.y);

		for(int y = a.y; y != b.y + dy; y += dy) {
			CaveMap[y][a.x] = FieldType::Stone;
		}

		for(int x = a.x; x != b.x + dx; x += dx) {
			CaveMap[a.y][x] = FieldType::Stone;
		}
	}

	Coord MapCoord(Coord const& coord) const {
		return Coord{ .x = coord.x - MinX, .y = coord.y - MinY };
	}

	int MinX, MaxX, MinY, MaxY, Width, Height;
	Coord SandSourceMapped;
	std::vector<std::vector<FieldType>> CaveMap;
	std::stack<Coord> SandPath{};
};

int main() {

	auto rawLines = readInputLines("input.txt");

	auto coords = parseInput(rawLines);

	int minX, minY, maxX, maxY;

	findExtremes(coords, minX, maxX, minY, maxY);

	minY = std::min(minY, 0);
	maxY += 2;

	int const sourceX = 500;
	int const sourceY = 0;

	int const floorDistanceFromSource = maxY - sourceY;

	minX = std::min(minX, sourceX - floorDistanceFromSource);
	maxX = std::max(maxX, sourceX + floorDistanceFromSource);

	Cave cave(minX, minY, maxX, maxY, { sourceX, sourceY });

	for(auto const& path : coords) {
		cave.AddStonePath(path);
	}

	// add floor
	cave.AddStonePath(std::vector<Coord>{Coord{ .x = minX, .y = maxY }, Coord{ .x = maxX, .y = maxY }});

	int i = 0;

	for(; cave.DropSand(); ++i);
	
	cave.PrintMap();
	std::cout << "Sand units: " << i << '\n';

	return 0;
}