#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
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


class Intervals {
public:
	void AddInterval(long long start, long long end) {
		std::cout << "Adding " << start << "..." << end << '\n';
		auto it = EndStarts.lower_bound(end);

		if(it == EndStarts.end() || it->second > end) {
			it = EndStarts.emplace(end, start).first;
			std::cout << "\tAdded new!\n";
		}
		else {
			if(it->second > start) {
				end = it->first;
				it->second = start;
				std::cout << "\tEnd = " << end << "\n";
				std::cout << "\tModified start to " << start << "\n";
			}
			else {
				std::cout << "\tFully contained by start " << it->second << "\n";
				return;
			}
		}

		auto rit = std::make_reverse_iterator(it);
		std::advance(rit, -1);

		do {
			std::advance(rit, 1);

			if(rit == EndStarts.rend()) {
				// no more intervals
				std::cout << "\tReached end" << "\n";
				break;
			}

			auto val = *rit;
			std::cout << "\tQuery: " << val.second << "..." << val.first << "\n";

			if(rit->first < start) {
				// the interval is too far
				std::cout << "\t\tToo far" << "\n";
				break;
			}

			auto const extension = rit->second;

			auto toBeRemoved = std::prev(rit.base());
			std::cout << "\t\tRemoving " << toBeRemoved->second << "..." << toBeRemoved->first << "\n";
			rit = std::make_reverse_iterator(std::prev(EndStarts.erase(toBeRemoved)));

			if(extension <= start) {
				// the interval is fully contained
				std::cout << "\t\tInterval contained, interval extended to " << extension << "\n";

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

struct Coord {
	long long x, y;
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

		if(BeaconCoord.y == y) {
			tryPushInterval(xMin, std::min(xMax, BeaconCoord.x - 1));
			tryPushInterval(std::max(xMin, BeaconCoord.x + 1), xMax);
		}
		else {
			tryPushInterval(xMin, xMax);
		}
	}

	long long GetManhattanDistance() const {
		return std::abs(SensorCoord.x - BeaconCoord.x) + std::abs(SensorCoord.y - BeaconCoord.y);
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

int main() {
	auto lines = getInputLines("input.txt");

	auto sensors = parseLines(lines);

	auto excludedIntervals = getExcludedIntervalsAtY(2000000, sensors);

	std::cout << excludedIntervals.GetIntervalCoverSize() << std::endl;

	return 0;
}