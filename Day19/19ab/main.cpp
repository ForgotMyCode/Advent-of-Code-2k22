#include <charconv>
#include <fstream>
#include <iostream>
#include <regex>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

void crash(std::string_view message) {
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

struct Price {
	int ores;
	int clay;
	int obsidian;
};

struct Blueprint {
	Price oreRobotCost;
	Price clayRobotCost;
	Price obsidianRobotCost;
	Price geodeRobotCost;
};

int strToInt(std::string_view str) {
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	return 0;
}

std::vector<Blueprint> parseBlueprints(std::vector<std::string> const& lines) {
	std::vector<Blueprint> blueprints;
	blueprints.reserve(lines.size());

	std::regex pattern("Blueprint [0-9]+: Each ore robot costs ([0-9]+) ore. Each clay robot costs ([0-9]+) ore. Each obsidian robot costs ([0-9]+) ore and ([0-9]+) clay. Each geode robot costs ([0-9]+) ore and ([0-9]+) obsidian.");

	std::smatch match;

	for(auto const& line : lines) {
		[[unlikely]]
		if(!std::regex_match(line, match, pattern)) {
			crash("Could not match regex to this line: \"" + line + "\".");
		}

		[[unlikely]]
		if(match.size() != 7) {
			crash("Expected 7 regex groups.");
		}

		std::string const& strOreRobotCostOre = match[1];
		std::string const& strClayRobotCostOre = match[2];
		std::string const& strObsidianRobotCostOre = match[3];
		std::string const& strObsidianRobotCostClay = match[4];
		std::string const& strGeodeRobotCostOre = match[5];
		std::string const& strGeodeRobotCostObsidian = match[6];

		int const oreRobotCostOre = strToInt(strOreRobotCostOre);
		int const clayRobotCostOre = strToInt(strClayRobotCostOre);
		int const obsidianRobotCostOre = strToInt(strObsidianRobotCostOre);
		int const obsidianRobotCostClay = strToInt(strObsidianRobotCostClay);
		int const geodeRobotCostOre = strToInt(strGeodeRobotCostOre);
		int const geodeRobotCostObsidian = strToInt(strGeodeRobotCostObsidian);

		Blueprint blueprint{
			.oreRobotCost = Price{.ores = oreRobotCostOre},
			.clayRobotCost = Price{.ores = clayRobotCostOre},
			.obsidianRobotCost = Price{.ores = obsidianRobotCostOre, .clay = obsidianRobotCostClay},
			.geodeRobotCost = Price{.ores = geodeRobotCostOre, .obsidian = geodeRobotCostObsidian}
		};

		blueprints.emplace_back(blueprint);
	}

	return blueprints;
}

struct Search {
	int ore{};
	int clay{};
	int obsidian{};
	int geode{};
	int oreRobots{};
	int clayRobots{};
	int obsidianRobots{};
	int geodeRobots{};
	int time{};

	bool operator==(Search const&) const noexcept = default;
};

template<>
struct std::hash<Search> {
	size_t operator()(Search const& search) const noexcept {
		static std::hash<int> hashInt{};

		return hashInt(search.ore) ^ hashInt(search.clay) ^ hashInt(search.obsidian) ^ hashInt(search.geode) ^ hashInt(search.oreRobots) ^ hashInt(search.clayRobots) ^ hashInt(search.obsidianRobots) ^ hashInt(search.geodeRobots) ^ hashInt(search.time);
	}
};

int evaluateBlueprint(Blueprint const& blueprint, int time) {
	std::stack<Search> ss;

	auto getTimeToReach = [](int currentAmount, int expectedAmount, int numberOfRobots, bool& isPossible) -> int {
		if(expectedAmount <= currentAmount) {
			isPossible = true;
			return 1;
		}

		if(numberOfRobots <= 0) {
			isPossible = false;
			return -1;
		}

		auto const amountDiff = expectedAmount - currentAmount;

		isPossible = true;
		return 1 + ((amountDiff + numberOfRobots - 1) / numberOfRobots);
	};

	auto getTimeToAfford = [&getTimeToReach](Price const& price, Search const& state, bool& isPossible) -> int {
		bool isPossibleOre;
		int oreTime = getTimeToReach(state.ore, price.ores, state.oreRobots, isPossibleOre);
		
		bool isPossibleClay;
		int clayTime = getTimeToReach(state.clay, price.clay, state.clayRobots, isPossibleClay);

		bool isPossibleObsidian;
		int obsidianTime = getTimeToReach(state.obsidian, price.obsidian, state.obsidianRobots, isPossibleObsidian);

		isPossible = isPossibleOre && isPossibleClay && isPossibleObsidian;

		return std::max({oreTime, clayTime, obsidianTime});
	};

	auto advanceTime = [](Search const& state, int deltaTime) -> Search {
		Search newState(state);

		newState.ore += state.oreRobots * deltaTime;
		newState.clay += state.clayRobots * deltaTime;
		newState.obsidian += state.obsidianRobots * deltaTime;
		newState.geode += state.geodeRobots * deltaTime;

		newState.time += deltaTime;

		return newState;
	};

	auto payPrice = [](Search& state, Price const& price) -> void {
		state.ore -= price.ores;
		state.clay -= price.clay;
		state.obsidian -= price.obsidian;
	};

	int maxGeode = 0;
	auto push = [&ss](Search&& state) mutable -> void {

		ss.push(std::move(state));
	};

	push(Search{ .oreRobots = 1, .time = 0 });

	do {
		auto state = ss.top();
		ss.pop();

		bool canMakeOreRobot, canMakeClayRobot, canMakeObsidianRobot, canMakeGeodeRobot;
		int timeToOreRobot = getTimeToAfford(blueprint.oreRobotCost, state, canMakeOreRobot);
		int timeToClayRobot = getTimeToAfford(blueprint.clayRobotCost, state, canMakeClayRobot);
		int timeToObsidianRobot = getTimeToAfford(blueprint.obsidianRobotCost, state, canMakeObsidianRobot);
		int timeToGeodeRobot = getTimeToAfford(blueprint.geodeRobotCost, state, canMakeGeodeRobot);

		if(canMakeOreRobot && state.time + timeToOreRobot < time - 2) {
			Search nextState = advanceTime(state, timeToOreRobot);
			payPrice(nextState, blueprint.oreRobotCost);
			++nextState.oreRobots;
			push(std::move(nextState));
		}

		if(canMakeClayRobot && state.time + timeToClayRobot < time - 2) {
			Search nextState = advanceTime(state, timeToClayRobot);
			payPrice(nextState, blueprint.clayRobotCost);
			++nextState.clayRobots;
			push(std::move(nextState));
		}

		if(canMakeObsidianRobot && state.time + timeToObsidianRobot < time - 2) {
			Search nextState = advanceTime(state, timeToObsidianRobot);
			payPrice(nextState, blueprint.obsidianRobotCost);
			++nextState.obsidianRobots;
			push(std::move(nextState));
		}

		if(canMakeGeodeRobot && state.time + timeToGeodeRobot < time) {
			Search nextState = advanceTime(state, timeToGeodeRobot);
			payPrice(nextState, blueprint.geodeRobotCost);
			++nextState.geodeRobots;
			push(std::move(nextState));

			int const remainingTime = time - nextState.time;

			int const query = remainingTime * nextState.geodeRobots + nextState.geode;

			maxGeode = std::max(maxGeode, query);
		}


	} while(!ss.empty());

	return maxGeode;
}

std::vector<int> evaluateBlueprints(std::vector<Blueprint> const& blueprints, int time) {
	std::vector<int> values;
	values.reserve(blueprints.size());

	for(int blueprintCount = 0;  auto const& blueprint : blueprints) {
		std::cout << "Evaluating blueprint " << (++blueprintCount) << "/" << blueprints.size() << '\n';
		values.emplace_back(evaluateBlueprint(blueprint, time));
		std::cout << "\t" << values.back() << '\n';
	}

	return values;
}

int main() {
	auto lines = readLines("input.txt");

	auto blueprints = parseBlueprints(lines);

	auto firstThreeBlueprints = std::vector<Blueprint>(blueprints.begin(), std::next(blueprints.begin(), 3));

	auto firstThreeBlueprintsValues = evaluateBlueprints(firstThreeBlueprints, 32);

	int firstThreeSums = 1;

	for(auto const& blueprintValue : firstThreeBlueprintsValues) {
		firstThreeSums *= blueprintValue;
	}

	std::cout << "Result 2: " << firstThreeSums << std::endl;

	auto blueprintValues = evaluateBlueprints(blueprints, 24);

	int sum = 0;

	for(int blueprintCount = 0;  auto const& blueprintValue : blueprintValues) {
		sum += (++blueprintCount) * blueprintValue;
	}

	std::cout << "Result: " << sum << '\n';


	return 0;
}