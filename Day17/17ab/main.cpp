#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

std::string readInput(std::string const& file) {
	std::ifstream input(file);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
	}

	std::string directions;

	[[unlikely]]
	if(!std::getline(input, directions)) {
		crash("Could not read input.");
	}

	input.close();

	return directions;
}

enum class FieldType : char {
	Air = '.',
	SolidStone = '#'
};

struct Shape {

	Shape() noexcept = default;

	Shape(std::vector<FieldType>&& buffer, int width, int height) noexcept :
		Width(width),
		Height(height),
		Buffer(buffer)
	{
		[[unlikely]]
		if(width * height != int(buffer.size())) {
			crash("Width * height is not buffer.size()");
		}
	}

	FieldType& operator()(int x, int y) noexcept {
		return Buffer[y * Width + x];
	}

	FieldType operator()(int x, int y) const noexcept {
		return Buffer[y * Width + x];
	}

	int Width{}, Height{};
	std::vector<FieldType> Buffer;
};

struct Cave : public Shape {

	Cave(int width) noexcept :
		Shape({}, width, 0),
		Ys(width, -1)
	{
		EmptyRow = std::vector(width, FieldType::Air);
	}

	void AddRow() noexcept {
		Buffer.insert(Buffer.end(), EmptyRow.begin(), EmptyRow.end());
		++Height;
	}

	void AddRows(int count) {
		for(int i = 0; i < count; ++i) {
			AddRow();
		}
	}

	bool IsColliding() const noexcept {
		if(ShapeX < 0 || ShapeY < 0 || ShapeX + CurrentShape.Width > Width) {
			return true;
		}

		for(int y = 0; y < CurrentShape.Height; ++y) {
			for(int x = 0; x < CurrentShape.Width; ++x) {
				
				if(CurrentShape(x, y) == FieldType::SolidStone && operator()(x + ShapeX, y + ShapeY) == FieldType::SolidStone) {
					return true;
				}

			}
		}

		return false;

	}

	bool ShiftX(int deltaX) noexcept {
		ShapeX += deltaX;
		if(IsColliding()) {
			ShapeX -= deltaX;
			return false;
		}
		return true;
	}

	bool ShiftY(int deltaY) noexcept {
		ShapeY += deltaY;
		if(IsColliding()) {
			ShapeY -= deltaY;
			return false;
		}
		return true;
	}

	void SpawnRock(Shape const& shape, int x, int y) noexcept {
		while(Height < y + shape.Height) {
			AddRow();
		}

		ShapeX = x;
		ShapeY = y;
		CurrentShape = shape;
	}

	void Solidify() noexcept {

		for(int y = 0; y < CurrentShape.Height; ++y) {
			for(int x = 0; x < CurrentShape.Width; ++x) {
				if(CurrentShape(x, y) == FieldType::SolidStone) {
					operator()(x + ShapeX, y + ShapeY) = FieldType::SolidStone;
					Ys[x + ShapeX] = y + ShapeY;
				}
			}
		}

		HighestRockY = std::max(HighestRockY, ShapeY + CurrentShape.Height - 1);

	}

	std::vector<int> GetRelativeYsView() const noexcept {
		std::vector<int> ret(Ys);
		for(auto& y : ret) {
			y -= HighestRockY;
		}
		return ret;
	}

	struct CacheElement {
		int shapeIdx;
		int directionIdx;
		std::vector<int> ysView;

		bool operator==(CacheElement const&) const noexcept = default;
	};

	struct CacheElementHash {

		size_t operator()(CacheElement const& element) const noexcept {
			static std::hash<int> hashInt{};
			size_t hash = hashInt(element.directionIdx) ^ hashInt(element.shapeIdx);

			for(auto const& y : element.ysView) {
				hash ^= hashInt(y);
			}
			return hash;
		}

	};

	template<bool isTerminatedOnCacheHit>
	int Simulate(std::vector<Shape> const& shapes, std::string const& directions, int steps) {

		std::unordered_map<CacheElement, int, CacheElementHash> cache;

		int shapeIdx = 0;
		int directionIdx = 0;

		auto nextShape = [&shapes, &shapeIdx]() mutable -> Shape const& {
			Shape const& shape = shapes[shapeIdx];
			shapeIdx = (shapeIdx + 1) % shapes.size();
			return shape;
		};

		auto nextDirection = [&directions, &directionIdx]() mutable -> char {
			auto const direction = directions[directionIdx];
			directionIdx = (directionIdx + 1) % directions.length();
			return direction;
		};

		for(int step = 0; step < steps; ++step) {

			SpawnRock(nextShape(), 2, HighestRockY + 4);

			do {

				char const windDirection = nextDirection();

				switch(windDirection) {
				case '>':
					ShiftX(1);
					break;
				case '<':
					ShiftX(-1);
					break;
				default:
					crash("Invalid direction");
				}

				if(!ShiftY(-1)) {
					Solidify();
					break;
				}

			} while(true);

			if constexpr(isTerminatedOnCacheHit) {

				auto relativeYsView = GetRelativeYsView();
					
				auto& previousStep = cache[CacheElement{ .shapeIdx = shapeIdx, .directionIdx = directionIdx, .ysView = relativeYsView }];

				if(previousStep != 0) {
					std::cout << "Cache hit!\n";
					return previousStep;
				}
				else {
					previousStep = step;
				}

			}

			HeightRecord.emplace_back(HighestRockY);

		}

		return -1;
	}

	void Print(int lines) const noexcept {
		for(int y = Height - 1; y > Height - 1 - lines; --y) {

			for(int x = 0; x < Width; ++x) {
				std::cout << std::to_underlying(operator()(x, y));
			}

			std::cout << '\n';
		}
	}

	std::vector<FieldType> EmptyRow;
	int ShapeX, ShapeY;
	Shape CurrentShape;
	int HighestRockY{ -1 };
	std::vector<int> HeightRecord;
	std::vector<int> Ys;
};

int main() {

	std::string directions = readInput("input.txt");

	FieldType air = FieldType::Air;
	FieldType stone = FieldType::SolidStone;

	std::vector<Shape> shapes{
		{ {stone, stone, stone, stone}, 4, 1}, // -
		{ {air, stone, air, stone, stone, stone, air, stone, air}, 3, 3}, // +
		{ {stone, stone, stone, air, air, stone, air, air, stone}, 3, 3}, // horizontally flipped L
		{ {stone, stone, stone, stone}, 1, 4}, // I
		{ {stone, stone, stone, stone}, 2, 2}, // square
	};

	Cave cave(7);

	cave.Simulate<false>(shapes, directions, 2022);

	std::cout << (1 + cave.HighestRockY) << std::endl;

	Cave cave2(7);


	long long const stepFrom = cave2.Simulate<true>(shapes, directions, 999999);

	long long const heightFrom = cave2.HeightRecord[stepFrom - 1];

	long long const heightTo = cave2.HeightRecord.back();

	long long const heightDiff = long long(heightTo - heightFrom);

	long long const stepTo = long long(cave2.HeightRecord.size());

	long long const stepSize = long long(stepTo - stepFrom);

	long long const query = 1000000000000LL - stepFrom;

	long long const queryMult = query / stepSize;

	long long const baseHeight = queryMult * heightDiff;

	long long const queryExtra = query - queryMult * stepSize;

	long long const heightExtra = cave2.HeightRecord[stepFrom + queryExtra - 1L];

	long long const extraHeight = heightExtra - heightFrom;

	std::cout << (1LL + heightFrom + baseHeight + extraHeight) << std::endl;

	return 0;
}