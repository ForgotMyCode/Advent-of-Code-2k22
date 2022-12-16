#include <algorithm>
#include <charconv>
#include <fstream>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

std::vector<std::string> getInputLines(std::string const& fileName) {
	std::fstream input(fileName);

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

long long strToInt(std::string_view str) {
	[[likely]]
	if(long long result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not convert " + std::string(str) + " to int.");
	return 0;
}

namespace {

	inline
	namespace v2 {
		class Intervals {
		public:
			std::vector<long long> GetFreeSpaces(long long min, long long max) const {
				if(HalfLines.empty()) {
					crash("Empty! Not defined");
				}

				int level = 0;

				long long emptinessStart = max + 1;

				std::vector<long long> result;

				for(auto const& [point, overlaps] : HalfLines) {

					if(level == 0) {
						for(long long i = std::max(min, emptinessStart); i <= std::min(max, point - 1); ++i) {
							result.emplace_back(i);
						}
					}

					level += overlaps;

					if(level == 0) {
						emptinessStart = point + 1;
					}
				}

				for(long long i = min; i < HalfLines.begin()->first; ++i) {
					result.emplace_back(i);
				}

				for(long long i = 1L + std::prev(HalfLines.end())->first; i <= max; ++i) {
					result.emplace_back(i);
				}

				return result;
			}


			bool AddInterval(long long start, long long end) {
				if(start > end) {
					return false;
				}

				HalfLines[start]++;
				HalfLines[end]--;

				return true;
			}

			long long GetIntervalCoverSize() const {
				int level = 0;

				long long lastLevel0Start = 0L;

				long long result = 0L;

				for(auto const& [point, overlaps] : HalfLines) {

					if(level == 0) {
						lastLevel0Start = point;
					}

					level += overlaps;

					if(level == 0) {
						result += point + 1 - lastLevel0Start;
					}
				}

				return result;
			}

			void Simplify() {
				if(HalfLines.empty()) {
					return;
				}

				[[unlikely]]
				if(HalfLines.begin()->second = false) {
					crash("Bad interval bounds");
				}

				int level = 0;

				for(auto it = HalfLines.begin(); it != HalfLines.end(); ++it) {
					
					auto const levelBefore = level;

					level += it->second;

					it->second = std::min(it->second, 1);
					it->second = std::max(it->second, -1);

					if(levelBefore != 0 && level != 0) {
						it = HalfLines.erase(it);
					}

					[[unlikely]]
					if(level < 0) {
						crash("Negative interval level");
					}

				}

				[[unlikely]]
				if(level != 0) {
					crash("Intervals not leveled");
				}
			}

		private:

			// point, isPositiveDirection
			std::map<long long, int> HalfLines;
		};
	}

	namespace v1 {
		class Intervals {
		public:
			std::vector<long long> GetFreeSpaces(long long min, long long max) const {
				std::vector<long long> ret;

				long long coverUntil = min;

				for(auto const& [end, start] : EndStarts) {
					for(; coverUntil < start; ++coverUntil) {
						if(coverUntil <= max) {
							ret.emplace_back(coverUntil);
						}
					}
					coverUntil = end + 1;
				}

				for(; coverUntil < max; ++coverUntil) {
					ret.emplace_back(coverUntil);
				}

				return ret;
			}

			void AddInterval(long long start, long long end) {
				//std::cout << "Adding " << start << "..." << end << '\n';
				auto it = EndStarts.lower_bound(end);

				if(it == EndStarts.end() || it->second > end) {
					it = EndStarts.emplace(end, start).first;
					//std::cout << "\tAdded new!\n";
				}
				else {
					if(it->second > start) {
						end = it->first;
						it->second = start;
						//std::cout << "\tEnd = " << end << "\n";
						//std::cout << "\tModified start to " << start << "\n";
					}
					else {
						//std::cout << "\tFully contained by start " << it->second << "\n";
						return;
					}
				}

				auto rit = std::make_reverse_iterator(it);
				std::advance(rit, -1);

				do {
					std::advance(rit, 1);

					if(rit == EndStarts.rend()) {
						// no more intervals
						//std::cout << "\tReached end" << "\n";
						break;
					}

					auto val = *rit;
					//std::cout << "\tQuery: " << val.second << "..." << val.first << "\n";

					if(rit->first < start) {
						// the interval is too far
						//std::cout << "\t\tToo far" << "\n";
						break;
					}

					auto const extension = rit->second;

					auto toBeRemoved = std::prev(rit.base());
					//std::cout << "\t\tRemoving " << toBeRemoved->second << "..." << toBeRemoved->first << "\n";
					rit = std::make_reverse_iterator(EndStarts.erase(toBeRemoved));

					if(extension <= start) {
						// the interval is fully contained
						//std::cout << "\t\tInterval contained, interval extended to " << extension << "\n";

						auto& currentStart = EndStarts[end];
						currentStart = std::min(currentStart, extension);
						break;
					}
				} while(true);
			}

			long long GetIntervalCoverSize() const {
				long long coverSize = 0L;

				for(auto const& [end, start] : EndStarts) {
					coverSize += end + 1 - start;
				}

				return coverSize;
			}

		private:
			std::map<long long, long long> EndStarts;
		};
	}
}

struct Coord {
	long long x, y;
};

long long getDirection(long long a, long long b) {
	if(a == b) {
		return 0;
	}

	return b > a ? 1L : -1L;
}

Coord getDirection(Coord const& a, Coord const& b) {
	Coord ret{};

	ret.x = getDirection(a.x, b.x);
	ret.y = getDirection(a.y, b.y);

	return ret;
}

long long getTriangleLength(Coord const& a, Coord const& b) {
	long long const xLength = std::abs(a.x - b.x);
	long long const yLength = std::abs(a.y - b.y);

	return std::max(xLength, yLength);
}

struct Line {
	Coord Origin;
	Coord Direction;
	long long Length;

	Line(Coord const& point1, Coord const& point2) :
		Origin(point1),
		Direction(getDirection(point1, point2)),
		Length(getTriangleLength(point1, point2))
	{}

	bool Intersects(Line const& other, Coord& intersection) const {
		auto const div = -Direction.x * other.Direction.y + other.Direction.x * Direction.y;

		if(div == 0) {
			return false;
		}

		auto const ia = -other.Direction.y;
		auto const ib = other.Direction.x;
		auto const ic = -Direction.y;
		auto const id = Direction.x;


		auto const lhsX = other.Origin.x - Origin.x;
		auto const lhsY = other.Origin.y - Origin.y;

		auto solutionX = ia * lhsX + ib * lhsY;
		auto solutionY = ic * lhsX + id * lhsY;


		if(solutionX % div != 0 || solutionY % div != 0) {
			return false;
		}

		solutionX /= div;
		solutionY /= div;

		if(solutionX < 0 || solutionX > Length || solutionY < 0 || solutionY > other.Length) {
			return false;
		}

		intersection.x = Origin.x + Direction.x * solutionX;
		intersection.y = Origin.y + Direction.y * solutionX;

		return true;
	}
};

struct Sensor {
	Sensor(Coord const& sensorCoord, Coord const& beaconCoord) :
		SensorCoord(sensorCoord), BeaconCoord(beaconCoord) 
	{}

	void ExcludeAtY(long long y, Intervals& out) const {
		auto const d = GetManhattanDistance();

		auto const dy = std::abs(SensorCoord.y - y);

		auto const xMin = -d + dy + SensorCoord.x;
		auto const xMax = d - dy + SensorCoord.x;

		auto tryPushInterval = [&out](long long xMin, long long xMax) mutable -> void {
			if(xMin <= xMax) {
				out.AddInterval(xMin, xMax);
			}
		};

		tryPushInterval(xMin, xMax);
	}

	long long GetManhattanDistance() const {
		return std::abs(SensorCoord.x - BeaconCoord.x) + std::abs(SensorCoord.y - BeaconCoord.y);
	}

	Coord GetPointAbove() const {
		return Coord{ SensorCoord.x, SensorCoord.y - GetManhattanDistance() - 1 };
	}

	Coord GetPointBelow() const {
		return Coord{ SensorCoord.x, SensorCoord.y + GetManhattanDistance() + 1 };
	}

	Coord GetPointLeft() const {
		return Coord{ SensorCoord.x - GetManhattanDistance() - 1, SensorCoord.y};
	}

	Coord GetPointRight() const {
		return Coord{ SensorCoord.x + GetManhattanDistance() + 1, SensorCoord.y };
	}

	Coord SensorCoord;
	Coord BeaconCoord;
};

std::vector<long long> extractInts(std::string_view str) {
	auto it = str.begin();

	static constexpr std::string_view digits("-0123456789");

	std::vector<long long> values;

	do {
		auto numStart = std::string_view(it, str.end()).find_first_of(digits, 0);
		
		if(numStart == std::string_view::npos) {
			break;
		}

		std::advance(it, numStart);

		std::string_view substr(it, str.end());

		auto numEnd = substr.find_first_not_of(digits);

		if(numEnd == std::string_view::npos) {
			numEnd = substr.length();
		}

		auto nextIt = std::next(it, numEnd);

		auto const value = strToInt(std::string_view(it, nextIt));

		values.emplace_back(value);

		it = nextIt;

	} while(true);

	return values;
}

Sensor parseLine(std::string_view line) {
	auto numbers = extractInts(line);

	[[unlikely]]
	if(numbers.size() != 4) {
		crash("4 numbers expected, got " + std::to_string(numbers.size()));
	}

	return Sensor(Coord{ .x = numbers[0], .y = numbers[1] }, Coord{ .x = numbers[2], .y = numbers[3] });
}

std::vector<Sensor> parseLines(std::vector<std::string> const& lines) {
	std::vector<Sensor> sensors;
	sensors.reserve(lines.size());

	for(auto const& line : lines) {
		sensors.emplace_back(parseLine(line));
	}

	return sensors;
}


Intervals getExcludedIntervalsAtY(long long y, std::vector<Sensor> const& sensors) {
	Intervals intervals{};

	for(auto const& sensor : sensors) {
		sensor.ExcludeAtY(y, intervals);
	}

	return intervals;
}

std::vector<Line> makeLines(std::vector<Sensor> const& sensors) {
	std::vector<Line> lines;

	for(auto const& sensor : sensors) {
		auto up = sensor.GetPointAbove();
		auto down = sensor.GetPointBelow();
		auto left = sensor.GetPointLeft();
		auto right = sensor.GetPointRight();

		lines.emplace_back(up, right);
		lines.emplace_back(right, down);
		lines.emplace_back(down, left);
		lines.emplace_back(left, up);
	}

	return lines;
}

std::unordered_set<long long> GetSuspiciousYs(std::vector<Line> const& lines) {
	std::unordered_set<long long> result;

	Coord intersection;

	for(size_t i = 0; i < lines.size(); ++i) {
		for(size_t j = i + 1; j < lines.size(); ++j) {
			if(lines[i].Intersects(lines[j], intersection)) {
				result.emplace(intersection.y);
			}
		}
	}

	return result;
}

int main() {
	auto lines = getInputLines("input.txt");

	auto sensors = parseLines(lines);

	auto geometry = makeLines(sensors);

	long long const limit = 4000000L;

	geometry.emplace_back(Line(Coord{ 0, 0 }, Coord{ limit, 0 }));
	geometry.emplace_back(Line(Coord{ 0, 0 }, Coord{ 0, limit }));
	geometry.emplace_back(Line(Coord{ 0, limit }, Coord{ limit, limit }));
	geometry.emplace_back(Line(Coord{ limit, 0 }, Coord{ limit, limit }));

	auto ys = GetSuspiciousYs(geometry);

	auto intervals = getExcludedIntervalsAtY(2000000L, sensors);

	std::cout << intervals.GetIntervalCoverSize() << std::endl;

	std::cout << "Suspicious ys: " << ys.size()  << '\n';


	for(auto const y : ys) {
		if(y < 0 || y > limit) {
			continue;
		}
		
		intervals = getExcludedIntervalsAtY(y, sensors);

		auto xs = intervals.GetFreeSpaces(0, limit);

		if(xs.size() > 0) {
			std::cout << "At y = " << y << " there are " << xs.size() << " xs.\n";

			for(auto const x : xs) {
				std::cout << '\t' << x << " -> " << (4000000L * x + y) << '\n';
			}
		}
	}

	return 0;
}