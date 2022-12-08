#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	assert(false);
	exit(1);
}

void markVisibleTreesInLine(std::vector<std::vector<int>> const& trees, std::vector<std::vector<bool>>& visibility, int beginX, int beginY, int endX, int endY, int deltaX, int deltaY) {

	int previous = -1;

	for(int x = beginX, y = beginY; x != endX && y != endY; x += deltaX, y += deltaY) {
		
		if(trees[y][x] > previous) {
			previous = trees[y][x];
			visibility[y][x] = true;
		}
	}
}

int lookAround(std::vector<std::vector<int>> const& trees, int x, int y) {

	std::stack<std::tuple<std::pair<int, int>, std::pair<int, int>, int>> ss;

	std::array<int, 4> pathLengths{};

	ss.push({ {x + 1, y}, {1, 0} , 0});
	ss.push({ {x - 1, y}, {-1, 0} , 1});
	ss.push({ {x, y + 1}, {0, 1} , 2});
	ss.push({ {x, y - 1}, {0, -1} , 3});

	int const query = trees[y][x];

	while(!ss.empty()) {
		auto const [coords, direction, key] = ss.top();
		auto const [xx, yy] = coords;
		auto const [dx, dy] = direction;
		ss.pop();

		if(xx < 0 || yy < 0 || xx >= trees.front().size() || yy >= trees.size()) {
			continue;
		}
		
		++pathLengths[key];

		if(trees[yy][xx] < query) {
			ss.push({ { xx + dx, yy + dy }, direction , key});
		}

	}

	int scenicScore = 1;

	for(auto const directionalScore : pathLengths) {
		scenicScore *= directionalScore;
	}

	return scenicScore;
}

int main() {
	
	std::fstream input("input.txt");

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open input file");
	}

	std::string line{};

	std::vector<std::vector<int>> trees;

	while(std::getline(input, line)) {
		std::vector<int> treeLine;
		treeLine.reserve(line.length());

		for(auto const& c : line) {			
			treeLine.emplace_back(int(c - '0'));
		}

		trees.emplace_back(std::move(treeLine));
	}

	input.close();

	std::vector<std::vector<bool>> visibleTrees{trees.size(), std::vector<bool>(trees.front().size())};

	int const endY = int(trees.size());
	int const endX = int(trees.front().size());

	for(int y = 0; y < endY; ++y) {
		markVisibleTreesInLine(trees, visibleTrees, 0, y, endX, y + 1, 1, 0);
		markVisibleTreesInLine(trees, visibleTrees, endX - 1, y, -1, y + 1, -1, 0);
	}

	for(int x = 0; x < endX; ++x) {
		markVisibleTreesInLine(trees, visibleTrees, x, 0, x + 1, endY, 0, 1);
		markVisibleTreesInLine(trees, visibleTrees, x, endY - 1, x + 1, -1, 0, -1);
	}

	int visibleTreesCount = 0;
	int bestScenicScore = 0;

	for(int y = 0; y < endY; ++y) {
		for(int x = 0; x < endX; ++x) {

			if(visibleTrees[y][x]) {
				++visibleTreesCount;
			}

			int const scenicScore = lookAround(trees, x, y);

			bestScenicScore = std::max(bestScenicScore, scenicScore);

			std::cout << scenicScore;
		}
		std::cout << '\n';
	}

	std::cout << "Visible trees: " << visibleTreesCount << '\n';
	std::cout << "Best scenic score: " << bestScenicScore << '\n';

	return 0;
}