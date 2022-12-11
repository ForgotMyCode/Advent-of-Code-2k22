#include <algorithm>
#include <cassert>
#include <charconv>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <string>
#include <vector>

struct DivisibilityTest {
	DivisibilityTest() noexcept = default;

	DivisibilityTest(int rhs) : Rhs(rhs) {};

	bool operator()(int lhs) const {
		return lhs % Rhs == 0;
	}

private:
	int Rhs{};
};

void crash(std::string const& message) {
	std::cerr << message << '\n';
	assert(false);
	exit(1);
}

struct ThrownItem {
	int WorryLevel{};
	int Recipient{};
};

class Monkey {
public:
	void AcquireItem(int item) {
		Items.emplace_back(item);
	}

	void SetWorryTransformation(std::function<int(int)>&& worryTransformation) {
		WorryTrasformation = std::move(worryTransformation);
	}

	void SetBoredTransformation(std::function<int(int)>&& boredTransformation) {
		BoredTransformation = std::move(boredTransformation);
	}

	void SetDecisionTest(DivisibilityTest&& decisionTest) {
		DecisionTest = std::move(decisionTest);
	}

	void ThrowItems(std::stack<ThrownItem>& thrownItems) {
		for(auto const& item : Items) {
			auto const newWorryLevel = BoredTransformation(WorryTrasformation(item));
			++InspectionCounter;
			auto const decisionTestResult = DecisionTest(newWorryLevel);
			auto const target = decisionTestResult ? TestPassTarget : TestFailTarget;

			thrownItems.push(ThrownItem{ .WorryLevel = newWorryLevel, .Recipient = target });
		}

		Items.clear();
	}

	void SetTargets(int pass, int fail) {
		TestPassTarget = pass;
		TestFailTarget = fail;
	}

	int GetInspectionCount() const {
		return InspectionCounter;
	}

private:
	std::vector<int> Items{};

	std::function<int(int)> WorryTrasformation{};

	std::function<int(int)> BoredTransformation{};

	int InspectionCounter{ 0 };

	DivisibilityTest DecisionTest{};

	int TestPassTarget{};
	int TestFailTarget{};
};

int strToInt(std::string_view str) {
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int " + std::string(str));

	return 0;
}

int main() {
	std::ifstream input("input.txt");

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file");
	}

	std::vector<Monkey> monkeys{};

	{
		std::string line{};
	
		std::vector<std::string> lines{};

		while(std::getline(input, line)) {
			lines.emplace_back(line);
		}
		
		auto monkeysRange = lines |
			std::views::chunk(7) |
			std::views::transform([](auto const& chunk) -> Monkey {

				auto items = chunk |
					std::views::drop(1) |
					std::views::take(1) |
					std::views::join |
					std::views::split(' ') |
					std::views::drop(4) |
					std::views::transform([](auto const& substr) -> std::string_view {
						return std::string_view(
							&*substr.begin(), 
							int(std::distance(substr.begin(), substr.end())) - (substr.back() == ',' ? 1 : 0)
						);
					}) |
					std::views::transform(&strToInt);
				
				Monkey monkey{};

				for(auto const& item : items) {
					monkey.AcquireItem(item);
				}

				auto worryTransform = chunk | // range of strings
					std::views::drop(2) | // range of strings
					std::views::take(1) | // range of strings
					std::views::join | // string
					std::views::split(' ') | // range of strings
					std::views::drop(5) | // range of strings
					std::views::chunk(3) | // range of ranges of strings
					std::views::transform([](auto const& equation) -> std::function<int(int)> {

						auto operand1range = *equation.begin();
						auto operatorRange = *std::next(equation.begin());
						auto operand2range = *std::next(equation.begin(), 2);

						std::function<int(int, int)> transform;

						char const equationOperator = *operatorRange.begin();

						switch(equationOperator) {
						case '+':
							transform = std::plus<int>{};
							break;
						case '*':
							transform = std::multiplies<int>{};
							break;
						default:
							crash("Invalid operand " + equationOperator);
						}

						std::string_view const equationOperand1(&*operand1range.begin(), std::distance(operand1range.begin(), operand1range.end()));
						std::string_view const equationOperand2(&*operand2range.begin(), std::distance(operand2range.begin(), operand2range.end()));

						bool isOperand1variable = false;
						bool isOperand2variable = false;

						int operand1value{};
						int operand2value{};

						if(equationOperand1 == "old") {
							isOperand1variable = true;
						}
						else {
							operand1value = strToInt(equationOperand1);
						}

						if(equationOperand2 == "old") {
							isOperand2variable = true;
						}
						else {
							operand2value = strToInt(equationOperand2);
						}

						return [isOperand1variable, isOperand2variable, operand1value, operand2value, transform](int value) -> int {
							return transform(isOperand1variable ? value : operand1value, isOperand2variable ? value : operand2value);
						};

					});

				monkey.SetWorryTransformation(*worryTransform.begin());

				auto decisionTest = chunk |
					std::views::drop(3) |
					std::views::take(1) |
					std::views::join |
					std::views::split(' ') |
					std::views::drop(5) |
					std::views::transform([](auto const& stringRange) -> std::string_view {
						return std::string_view(&*stringRange.begin(), std::distance(stringRange.begin(), stringRange.end())); 
					}) |
					std::views::transform(&strToInt);

				monkey.SetDecisionTest(*decisionTest.begin());

				monkey.SetBoredTransformation([](int x) {return x / 3; });

				auto testTargets = chunk |
					std::views::drop(4) |
					std::views::take(2) |
					std::views::transform([](auto const& line) -> int {
						auto lastNumber = line |
						std::views::split(' ') |
						std::views::drop(9) |
						std::views::transform([](auto const& stringRange) -> std::string_view {
							return std::string_view(&*stringRange.begin(), std::distance(stringRange.begin(), stringRange.end())); 
						}) |
						std::views::transform(&strToInt);

						return *lastNumber.begin();
					});

				int const testPassTarget = *testTargets.begin();
				int const testFailTarget = *std::next(testTargets.begin());

				monkey.SetTargets(testPassTarget, testFailTarget);

				return monkey;
			});

		monkeys.insert(monkeys.end(), monkeysRange.begin(), monkeysRange.end());
	}
	input.close();

	int const rounds = 20;

	std::stack<ThrownItem> thrownItems{};

	for(int round = 0; round < 20; ++round) {
		std::cout << "Round: " << round << '\n';
		
		for(int i = 0;  auto & monkey : monkeys) {
			monkey.ThrowItems(thrownItems);

			while(!thrownItems.empty()) {
				auto const [item, recipient] = thrownItems.top();
				thrownItems.pop();

				monkeys[recipient].AcquireItem(item);

				std::cout << "\tMonkey " << i << " throws item " << item << " to Monkey " << recipient << ".\n";
			}
			++i;
		}

		for(int i = 0;  auto const& monkey : monkeys) {

			std::cout << "[" << i++ << "]Inspections: " << monkey.GetInspectionCount() << '\n';
		}

	}

	std::vector<Monkey> sortedMonkeys(monkeys);

	std::sort(
		sortedMonkeys.begin(),
		sortedMonkeys.end(),
		[](Monkey const& a, Monkey const& b) -> bool {
		return a.GetInspectionCount() > b.GetInspectionCount();
	});


	std::cout << "Monkey business level: " << (sortedMonkeys[0].GetInspectionCount() *  sortedMonkeys[1].GetInspectionCount()) << std::endl;

	return 0;
}