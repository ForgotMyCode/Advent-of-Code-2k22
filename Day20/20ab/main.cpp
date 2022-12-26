#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using bigInt = long long;

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

int strToInt(std::string_view str) {
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int: " + std::string(str));
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

std::vector<int> parseLines(std::vector<std::string> const& lines) {
	std::vector<int> ret;

	for(auto const& line : lines) {
		ret.emplace_back(strToInt(line));
	}

	return ret;
}

template<typename T>
void shift(std::vector<T>& elements, std::vector<int>& originalElementIdx, std::vector<int>& whereIs, T index, T offset) {
	T targetIndex = T(index + offset);
	T const n = T(elements.size());

	if(targetIndex < T(0)) {
		T const div = targetIndex / (n - T(1));
		targetIndex -= (div - T(1)) * (n - T(1));
	}
	if(targetIndex >= n) {
		auto extra = targetIndex - n + T(1);
		T const div = extra / (n - T(1));
		targetIndex -= (div + T(1)) * (n - T(1));
	}

	if(index == targetIndex) {
		return;
	}

	int const advanceOffset = targetIndex < index ? -1 : 1;

	for(int i = int(index); i != int(targetIndex); i += advanceOffset) {
		auto advancedI = i + advanceOffset;

		std::swap(elements[i], elements[advancedI]);
		std::swap(originalElementIdx[i], originalElementIdx[advancedI]);
		whereIs[originalElementIdx[i]] = i;
		whereIs[originalElementIdx[advancedI]] = advancedI;
	}

}

template<typename T>
T getCircular(std::vector<T>& elements, T index) {
	T const n = T(elements.size());

	T const target = index % n;

	return elements[target];
}

template<typename T>
void printVector(std::vector<T> const & elements) {
	for(auto const& element : elements) {
		std::cout << element << ", ";
	}
	std::cout << '\n';
}

template<typename T>
void decrypt(std::vector<T>& elements, std::vector<int>& originalElementIdx, std::vector<int>& whereIs) {

	for(int i = 0; i < int(elements.size()); ++i) {
		shift(elements, originalElementIdx, whereIs, T(whereIs[i]), T(elements[whereIs[i]]));
	}
}

int main() {

	auto lines = readLines("input.txt");

	auto elements = parseLines(lines);
	std::vector<bigInt> biggerElements(elements.begin(), elements.end());

	std::vector<int> originalElementIdx(elements.size());
	std::vector<int> whereIs(elements.size());
	std::iota(originalElementIdx.begin(), originalElementIdx.end(), 0);
	std::iota(whereIs.begin(), whereIs.end(), 0);
	std::vector<int> originalElementIdx2(originalElementIdx);
	std::vector<int> whereIs2(whereIs);

	decrypt(elements, originalElementIdx, whereIs);

	int zeroIndex = int(std::distance(elements.begin(), std::find(elements.begin(), elements.end(), 0)));

	int sum = 0;

	for(auto const& x : { 1000, 2000, 3000 }) {
		auto y = getCircular(elements, zeroIndex + x);
		sum += y;
	}

	std::cout << "Result 1: " << sum << std::endl;

	for(auto& element : biggerElements) {
		element *= 811589153LL;
	}

	for(int i = 0; i < 10; ++i) {
		decrypt(biggerElements, originalElementIdx2, whereIs2);
	}

	int zeroIndex2 = int(std::distance(biggerElements.begin(), std::find(biggerElements.begin(), biggerElements.end(), 0LL)));

	bigInt sum2{};

	for(auto const& x : { 1000, 2000, 3000 }) {
		auto y = getCircular(biggerElements, bigInt(zeroIndex2 + x));
		sum2 += bigInt(y);
	}

	std::cout << "Result 2: " << sum2 << std::endl;

	return 0;
}