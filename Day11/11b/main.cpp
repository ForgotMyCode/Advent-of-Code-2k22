#include <algorithm>
#include <cassert>
#include <charconv>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>
#include <stack>
#include <string>
#include <vector>

struct DivisibilityTest {
	DivisibilityTest() noexcept = default;

	DivisibilityTest(long long rhs) : Rhs(rhs) {};

	bool operator()(long long lhs) const {
		return lhs % Rhs == 0L;
	}

	long long GetRhs() const {
		return Rhs;
	}

private:
	long long Rhs{};
};

void crash(std::string const& message) {
	std::cerr << message << '\n';
	assert(false);
	exit(1);
}

struct ThrownItem {
	long long WorryLevel{};
	int Recipient{};
};

class Monkey {
public:
	void AcquireItem(long long item) {
		Items.emplace_back(item);
	}

	void SetWorryTransformation(std::function<long long(long long)>&& worryTransformation) {
		WorryTrasformation = std::move(worryTransformation);
	}

	void SetBoredTransformation(std::function<long long(long long)>&& boredTransformation) {
		BoredTransformation = std::move(boredTransformation);
	}

	void SetDecisionTest(DivisibilityTest&& decisionTest) {
		DecisionTest = std::move(decisionTest);
		modulo = std::lcm(modulo, decisionTest.GetRhs());
	}

	void ThrowItems(std::stack<ThrownItem>& thrownItems) {
		for(auto const& item : Items) {
			auto const newWorryLevel = WorryTrasformation(item) % modulo;
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

	long long GetInspectionCount() const {
		return InspectionCounter;
	}

private:
	std::vector<long long> Items{};

	std::function<long long(long long)> WorryTrasformation{};

	std::function<long long(long long)> BoredTransformation{};

	long long InspectionCounter{ 0 };

	DivisibilityTest DecisionTest{};

	int TestPassTarget{};
	int TestFailTarget{};

	static long long modulo;
};

long long Monkey::modulo = 1;

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
					std::views::transform([](auto const& equation) -> std::function<long long(long long)> {

						auto operand1range = *equation.begin();
						auto operatorRange = *std::next(equation.begin());
						auto operand2range = *std::next(equation.begin(), 2);

						std::function<long long(long long, long long)> transform;

						char const equationOperator = *operatorRange.begin();

						switch(equationOperator) {
						case '+':
							transform = std::plus<long long>{};
							break;
						case '*':
							transform = std::multiplies<long long>{};
							break;
						default:
							crash("Invalid operand " + equationOperator);
						}

						std::string_view const equationOperand1(&*operand1range.begin(), std::distance(operand1range.begin(), operand1range.end()));
						std::string_view const equationOperand2(&*operand2range.begin(), std::distance(operand2range.begin(), operand2range.end()));

						bool isOperand1variable = false;
						bool isOperand2variable = false;

						long long operand1value{};
						long long operand2value{};

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

						return [isOperand1variable, isOperand2variable, operand1value, operand2value, transform](long long value) -> long long {
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

				monkey.SetBoredTransformation([](long long x) -> long long {return x / 1L; });

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

	int const rounds = 10000;

	std::stack<ThrownItem> thrownItems{};

	for(int round = 0; round < rounds; ++round) {
		std::cout << "Round: " << round << '\n';
		
		for(int i = 0;  auto & monkey : monkeys) {
			monkey.ThrowItems(thrownItems);

			while(!thrownItems.empty()) {
				auto const [item, recipient] = thrownItems.top();
				thrownItems.pop();

				monkeys[recipient].AcquireItem(item);

				//std::cout << "\tMonkey " << i << " throws item " << item << " to Monkey " << recipient << ".\n";
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