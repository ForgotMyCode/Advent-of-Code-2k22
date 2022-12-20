#include <array>
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

std::vector<std::string> readLines(std::string const& file) {
	std::ifstream input(file);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file!");
	}

	std::vector<std::string> lines;

	{
		std::string line;

		while(std::getline(input, line)) {
			lines.emplace_back(line);
		}
	}

	input.close();

	return lines;
}

using vec3 = std::tuple<int, int, int>;

static constexpr std::array<vec3, 6> lookAround{ {
	{-1, 0, 0},
	{1, 0, 0},
	{0, -1, 0},
	{0, 1, 0},
	{0, 0, -1},
	{0, 0, 1}
} };

int strToInt(std::string_view str) {

	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int " + std::string(str));
	return 0;
}

std::vector<vec3> parseCubes(std::vector<std::string> const& input) {
	auto cubesRange = input | 
		std::views::transform([](auto const& cubeString) {
			auto parsedInts = cubeString |
				std::views::split(std::string_view(",")) |
				std::views::transform([](auto const& rawIntRange){return strToInt(std::string_view(rawIntRange.begin(), rawIntRange.end()));});

			[[unlikely]]
			if(std::ranges::distance(parsedInts) != 3) {
				crash("Did not get 3D data as expected");
			}

			return vec3(*parsedInts.begin(), *std::next(parsedInts.begin()), *std::next(parsedInts.begin(), 2));
		});

	return std::vector<vec3>(cubesRange.begin(), cubesRange.end());
}

struct S {
	int x;
};

void getExtremes(std::vector<vec3> const& cubes, vec3& min, vec3& max) {

	min = cubes.front();
	max = min;

	auto& [minX, minY, minZ] = min;
	auto& [maxX, maxY, maxZ] = max;

	for(auto const& cube : cubes) {
		auto const& [x, y, z] = cube;

		minX = std::min(minX, x);
		minY = std::min(minY, y);
		minZ = std::min(minZ, z);

		maxX = std::max(maxX, x);
		maxY = std::max(maxY, y);
		maxZ = std::max(maxZ, z);
	}

}

template<typename Functor>
void forEach(vec3& vector, Functor transform) {
	auto& [x, y, z] = vector;
	x = transform(x);
	y = transform(y);
	z = transform(z);
}

struct VoxelGrid {

	VoxelGrid(vec3 const& min, vec3 const& max) :
		Min(min),
		Max(max),
		SizeX(1 + std::get<0>(max) - std::get<0>(min)),
		SizeY(1 + std::get<1>(max) - std::get<1>(min)),
		SizeZ(1 + std::get<2>(max) - std::get<2>(min))
	{
		Cubes = std::vector<std::vector<std::vector<bool>>>(SizeZ, std::vector<std::vector<bool>>(SizeY, std::vector<bool>(SizeX)));
		Neighbors = std::vector<std::vector<std::vector<int>>>(SizeZ, std::vector<std::vector<int>>(SizeY, std::vector<int>(SizeX)));
	}

	void AddCubeAt(vec3 const& position) noexcept {
		Cubes[std::get<2>(position) - std::get<2>(Min)][std::get<1>(position) - std::get<1>(Min)][std::get<0>(position) - std::get<0>(Min)] = true;
	}

	int CountNeighbors() noexcept {
		int count = 0;
		
		for(int z = 0; z < SizeZ; ++z) {
			for(int y = 0; y < SizeY; ++y) {
				for(int x = 0; x < SizeX; ++x) {
					
					for(auto const& [dx, dy, dz] : lookAround) {
						int const xx = x + dx;
						int const yy = y + dy;
						int const zz = z + dz;

						if(xx < 0 || yy < 0 || zz < 0 || xx >= SizeX || yy >= SizeY || zz >= SizeZ) {
							continue;
						}

						if(Cubes[zz][yy][xx]) {

							if(!Cubes[z][y][x]) {
								++count;
							}

							++Neighbors[z][y][x];
						}
					}

				}
			}
		}

		return count;
	}

	int CountReachable() {

		auto visited = std::vector<std::vector<std::vector<bool>>>(SizeZ, std::vector<std::vector<bool>>(SizeY, std::vector<bool>(SizeX)));

		int count = 0;

		std::stack<vec3> ss;

		ss.emplace(0, 0, 0);
		visited[0][0][0] = true;

		while(!ss.empty()) {
			auto const [x, y, z] = ss.top();
			ss.pop();

			count += Neighbors[z][y][x];

			for(auto const& [dx, dy, dz] : lookAround) {
				int const xx = x + dx;
				int const yy = y + dy;
				int const zz = z + dz;

				if(xx < 0 || yy < 0 || zz < 0 || xx >= SizeX || yy >= SizeY || zz >= SizeZ) {
					continue;
				}

				if(!Cubes[zz][yy][xx] && !visited[zz][yy][xx]) {

					visited[zz][yy][xx] = true;

					ss.emplace(xx, yy, zz);
				}
			}
		}

		return count;
	}

	std::vector<std::vector<std::vector<bool>>> Cubes;
	std::vector<std::vector<std::vector<int>>> Neighbors;


	vec3 Min, Max;
	int SizeX, SizeY, SizeZ;
};

int main() {
	auto lines = readLines("input.txt");

	auto cubes = parseCubes(lines);

	vec3 min, max;

	getExtremes(cubes, min, max);

	forEach(min, [](int x) { return x - 1; });
	forEach(max, [](int x) { return x + 1; });

	VoxelGrid voxelGrid(min, max);

	for(auto const& cube : cubes) {
		voxelGrid.AddCubeAt(cube);
	}

	auto const surface = voxelGrid.CountNeighbors();

	std::cout << surface << std::endl;

	auto const reachableSurface = voxelGrid.CountReachable();

	std::cout << reachableSurface << std::endl;

	return 0;
}