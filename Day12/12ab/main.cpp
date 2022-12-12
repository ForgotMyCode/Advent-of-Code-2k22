#include <array>
#include <cassert>
#include <fstream>
#include <format>
#include <iostream>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message;
	assert(false);
	exit(1);
}

std::vector<std::string> readInput(std::string const& fileName) {
	std::ifstream input(fileName);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
	}

	std::vector<std::string> rawMap;

	std::string line;

	while(std::getline(input, line)) {
		rawMap.emplace_back(line);
	}

	input.close();

	return rawMap;
}

std::vector<std::vector<int>> parseMap(std::vector<std::string> const& rawMap, int& startX, int& startY, int& endX, int& endY) {
	
	std::vector<std::vector<int>> map{};
	map.reserve(rawMap.size());
	
	for(int y = 0; auto const& rawLine : rawMap) {
		std::vector<int> line{};
		line.reserve(rawMap.size());

		for(int x = 0; auto const c : rawLine) {
			int height = -1;

			if(c == 'S') {
				startX = x;
				startY = y;
				height = 0;
			}
			else if(c == 'E') {
				endX = x;
				endY = y;
				height = int('z' - 'a');
			}
			else {
				height = int(c - 'a');
			}

			line.emplace_back(height);

			++x;
		}

		map.emplace_back(std::move(line));

		++y;
	}

	return map;
}

static constexpr std::array<std::array<int, 2>, 4> directions{ {
	{1, 0},
	{-1, 0},
	{0, 1},
	{0, -1}
} };

struct SearchNode {
	int x;
	int y;
};

struct Boolean {
	Boolean() noexcept = default;

	Boolean(bool value) noexcept : Value(value) {}

	operator bool() noexcept {
		return Value;
	}

private:
	bool Value{};
};

void printMap(std::vector<std::vector<int>> const& map) {
	for(auto const& line : map) {
		for(auto const& element : line) {
			std::cout << std::format("{:4}", element == std::numeric_limits<int>::max() ? -2 : element);
		}

		std::cout << '\n';

	}
}

int getShortestPath(std::vector<std::vector<int>> const& map, int startX, int startY, int endX, int endY, std::vector<std::vector<int>>& shortestPaths, bool(*heightCondition)(int, int) = [](int targetHeight, int currentHeight) {return targetHeight <= currentHeight + 1; }) {
	std::vector<std::vector<Boolean>> isEnqueueds(map.size(), std::vector<Boolean>(map.front().size(), false));

	std::queue<SearchNode> bfs;

	auto enqueue = [&shortestPaths, &isEnqueueds, &bfs](int x, int y, int pathLength) mutable -> void {
		auto& shortestPath = shortestPaths[y][x];

		if(pathLength < shortestPath) {
			shortestPath = pathLength;

			auto& isEnqueued = isEnqueueds[y][x];

			if(!isEnqueued) {
				isEnqueued = true;
				bfs.push(SearchNode{ .x = x, .y = y });
			}
		}
	};

	auto dequeue = [&bfs, &shortestPaths, &isEnqueueds]() mutable -> std::tuple<int, int, int> {
		auto const [x, y] = bfs.front();
		bfs.pop();

		isEnqueueds[y][x] = false;

		return { x, y, shortestPaths[y][x]};
	};

	auto isInMap = [&map](int x, int y) -> bool {
		return x >= 0 && y >= 0 && y < int(map.size()) && x < int(map.front().size());
	};

	enqueue(startX, startY, 0);

	do {
		auto const [x, y, pathLength] = dequeue();

		if(x == endX && y == endY) {
			printMap(shortestPaths);
			return pathLength;
		}

		int const currentHeight = map[y][x];

		for(auto const& [dx, dy] : directions) {
			
			int const targetX = x + dx;
			int const targetY = y + dy;

			if(!isInMap(targetX, targetY)) {
				continue;
			}

			int const targetHeight = map[targetY][targetX];

			if(heightCondition(targetHeight, currentHeight)) {
				enqueue(targetX, targetY, pathLength + 1);
			}

		}

	} while(!bfs.empty());
	
	printMap(shortestPaths);
	return -1;
}

int getShortestPath(std::vector<std::vector<int>> const& map, int startX, int startY, int endX, int endY) {
	std::vector<std::vector<int>> shortestPaths(map.size(), std::vector<int>(map.front().size(), std::numeric_limits<int>::max()));

	return getShortestPath(map, startX, startY, endX, endY, shortestPaths);
}

int getShortestPathFromUnknownStart(std::vector<std::vector<int>> const& map, int endX, int endY) {
	std::vector<std::vector<int>> shortestPaths(map.size(), std::vector<int>(map.front().size(), std::numeric_limits<int>::max()));

	getShortestPath(map, endX, endY, -1, -1, shortestPaths, [](int targetHeight, int currentHeight) {return targetHeight >= currentHeight - 1; });

	int shortestPath = std::numeric_limits<int>::max();

	for(int y = 0; auto const& line : map) {
		for(int x = 0; auto const& element : line) {
			
			if(map[y][x] == 0) {
				shortestPath = std::min(shortestPath, shortestPaths[y][x]);
			}

			++x;
		}

		++y;
	}

	return shortestPath;
}

int main() {

	std::vector<std::vector<int>> map;

	int startX = -1;
	int startY = -1;
	int endX = -1;
	int endY = -1;
	
	{
		auto rawMap = readInput("input.txt");
	
		map = parseMap(rawMap, startX, startY, endX, endY);
	}

	int const shortestPath = getShortestPath(map, startX, startY, endX, endY);
	std::cout << '\n';
	int const shortestPathFromAnywhere = getShortestPathFromUnknownStart(map, endX, endY);

	std::cout << "Shortest path: " << shortestPath << std::endl;

	std::cout << "Shortest path from anywhere: " << shortestPathFromAnywhere << std::endl;

	return 0;

}