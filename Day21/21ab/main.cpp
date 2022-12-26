#include <charconv>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using Tint = long long;
using Tid = long long;

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

Tint strToInt(std::string_view message) {
	[[likely]]
	if(Tint result; std::from_chars(message.data(), message.data() + message.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int from: " + std::string(message));
	return 0;
}

std::vector<std::string> readLines(std::string const& file) {
	std::ifstream input(file);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
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

namespace {
	std::unordered_map<Tid, std::string> idToStrMap;
	std::unordered_map<std::string, Tid> strToIdMap;
}

Tid getMonkeyId(std::string const& name) {
	[[unlikely]]
	if(strToIdMap.count(name) == 0) {
		crash("Monkey with name " + name + " does not exist!");
	}

	return strToIdMap[name];
}

std::string const& getMonkeyName(Tid id) {
	[[unlikely]]
	if(idToStrMap.count(id) == 0) {
		crash("Monkey with id " + std::to_string(id) + " does not exist!");
	}

	return idToStrMap[id];
}

void registerMonkey(Tid id, std::string const& name) {
	idToStrMap[id] = name;
	strToIdMap[name] = id;
}



struct Monkey;

using Tdependency = std::variant<Monkey, Tint>;

struct Monkey {

	Tid Id{};

	Tint LeftValue{}, RightValue{};
	Tint Solution{};

	Tint Depth{};

	std::vector<Tid> BeforeDependencies{};
	std::vector<Tid> AfterDependencies{};
	std::unordered_set<Tid> UnsatistiedDependencies{};
	std::function<Tint(Tint, Tint)> Operation{};
	std::function<Tint(Tint, Tint)> InverseParentLeft{};
	std::function<Tint(Tint, Tint)> InverseParentRight{};
};

std::vector<std::string> parseLine(std::string const& line, Monkey& monkey) {
	auto r = line |
		std::views::split(std::string_view(" "));

	auto const nParts = std::ranges::distance(r);

	auto nameRange = *r.begin();

	std::string_view name(nameRange.begin(), std::prev(nameRange.end()));

	registerMonkey(monkey.Id, std::string(name));

	std::vector<std::string> dependencies;

	if(nParts == 2) {
		auto numberRange = *std::next(r.begin());

		std::string_view numberStr(numberRange.begin(), numberRange.end());

		Tint const number = strToInt(numberStr);

		monkey.Operation = [number](Tint, Tint) -> Tint { return number; };
	}
	else if(nParts == 4) {
		auto param1range = *std::next(r.begin());
		auto operatorRange = *std::next(r.begin(), 2);
		auto param2range = *std::next(r.begin(), 3);

		std::string const param1name(param1range.begin(), param1range.end());
		std::string_view const operatorName(operatorRange.begin(), operatorRange.end());
		std::string const param2name(param2range.begin(), param2range.end());

		dependencies.emplace_back(param1name);
		dependencies.emplace_back(param2name);

		switch(operatorName[0]) {
		case '+':
			monkey.Operation = std::plus<Tint>{};
			monkey.InverseParentLeft = std::minus<Tint>{};
			monkey.InverseParentRight = std::minus<Tint>{};
			break;
		case '-':
			monkey.Operation = std::minus<Tint>{};
			monkey.InverseParentLeft = [](Tint parent, Tint left) -> Tint {return -parent + left;};
			monkey.InverseParentRight = std::plus<Tint>{};
			break;
		case '*':
			monkey.Operation = std::multiplies<Tint>{};
			monkey.InverseParentLeft = std::divides<Tint>{};
			monkey.InverseParentRight = std::divides<Tint>{};
			break;
		case '/':
			monkey.Operation = std::divides<Tint>{};
			monkey.InverseParentLeft = [](Tint parent, Tint left) -> Tint {return left / parent;};
			monkey.InverseParentRight = std::multiplies<Tint>{};
			break;
		default:
			crash("Unknown operator");
		}
	
	}
	else [[unlikely]] {
		crash("Invalid number of parameters: " + std::to_string(nParts));
	}

	[[unlikely]]
	if(!dependencies.empty() && dependencies.size() != 2) {
		crash("Expected 0 or 2 dependencies!");
	}

	return dependencies;
}

std::vector<Tid> parseDependencies(std::vector<std::string> const& rawDependencies) {
	std::vector<Tid> dependencies;
	dependencies.reserve(rawDependencies.size());

	for(auto const& dependency : rawDependencies) {
		dependencies.emplace_back(getMonkeyId(dependency));
	}

	return dependencies;
}

std::vector<Monkey> parseInput(std::vector<std::string> const& lines) {
	std::vector<Monkey> monkeys(lines.size());

	std::vector<std::vector<std::string>> dependencies;
	dependencies.reserve(lines.size());

	for(Tid i = 0; i < Tid(monkeys.size()); ++i) {
		Monkey& monkey = monkeys[i];
		monkey.Id = i;

		dependencies.emplace_back(parseLine(lines[i], monkey));
	}

	for(Tid i = 0; i < Tid(monkeys.size()); ++i) {
		Monkey& monkey = monkeys[i];

		monkey.BeforeDependencies = parseDependencies(dependencies[i]);

		monkey.UnsatistiedDependencies.insert(monkey.BeforeDependencies.begin(), monkey.BeforeDependencies.end());

		for(auto const& dependency : monkey.BeforeDependencies) {
			monkeys[dependency].AfterDependencies.emplace_back(monkey.Id);

			[[unlikely]]
			if(monkeys[dependency].AfterDependencies.size() > 1) {
				crash("Expected max 1 after dependency");
			}
		}
	}

	return monkeys;
}

void solveMonkeys(std::vector<Monkey>& monkeys) {
	std::stack<Tid> activeMonkeys;

	auto pushIfActive = [&activeMonkeys](Monkey const& monkey) mutable -> void {
		if(monkey.UnsatistiedDependencies.empty()) {
			activeMonkeys.push(monkey.Id);
		}
	};

	for(auto const& monkey : monkeys) {
		pushIfActive(monkey);
	}

	while(!activeMonkeys.empty()) {
		auto const id = activeMonkeys.top();
		activeMonkeys.pop();

		Monkey& monkey = monkeys[id];

		monkey.Solution = monkey.Operation(monkey.LeftValue, monkey.RightValue);

		for(auto const& targetMonkeyId : monkey.AfterDependencies) {
			Monkey& targetMonkey = monkeys[targetMonkeyId];

			targetMonkey.UnsatistiedDependencies.erase(monkey.Id);

			targetMonkey.Depth = std::max(targetMonkey.Depth, monkey.Depth + 1);

			if(targetMonkey.BeforeDependencies[0] == monkey.Id) {
				targetMonkey.LeftValue = monkey.Solution;
			}
			else if(targetMonkey.BeforeDependencies[1] == monkey.Id) {
				targetMonkey.RightValue = monkey.Solution;
			}
			else [[unlikely]] {
				crash("Expected a dependency");
			}

			if(targetMonkey.UnsatistiedDependencies.empty()) {
				activeMonkeys.push(targetMonkeyId);
			}
		}
	}
}

Tint solveHuman(std::vector<Monkey> const& monkeys) {
	std::stack<Tid> humanStack;

	humanStack.push(getMonkeyId("humn"));

	auto const rootId = getMonkeyId("root");

	while(humanStack.top() != rootId) {
		auto const nextMonkey = monkeys[humanStack.top()].AfterDependencies.front();
		humanStack.push(nextMonkey);
	}

	auto isLeft = [&monkeys](Tid monkeyId) {
		return monkeys[monkeys[monkeyId].AfterDependencies.front()].BeforeDependencies.front() == monkeyId;
	};

	Monkey const& root = monkeys[rootId];
	humanStack.pop();

	Tint result{};

	if(isLeft(humanStack.top())) {
		result = root.RightValue;
	}
	else {
		result = root.LeftValue;
	}

	while(humanStack.size() > 1) {
		Monkey const& monkey = monkeys[humanStack.top()];
		humanStack.pop();

		bool const isLeftMonkey = isLeft(humanStack.top());

		if(isLeftMonkey) {
			result = monkey.InverseParentRight(result, monkey.RightValue);
		}
		else {
			result = monkey.InverseParentLeft(result, monkey.LeftValue);
		}
	}

	return result;
}

int main() {

	auto lines = readLines("input.txt");

	auto monkeys = parseInput(lines);

	solveMonkeys(monkeys);

	auto rootMonkeyId = getMonkeyId("root");

	std::cout << "Result 1: " << monkeys[rootMonkeyId].Solution << std::endl;

	auto human = solveHuman(monkeys);

	std::cout << "Result 2: " << human << std::endl;

	return 0;
}
