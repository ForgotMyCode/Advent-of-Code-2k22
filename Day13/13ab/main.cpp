#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stack>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

void crash(std::string const& message) {
	std::cerr << message << '\n';
	std::terminate();
}

std::vector<std::string> readInput(std::string const& fileName) {
	std::ifstream input(fileName);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file.");
	}

	std::vector<std::string> lines{};

	{
		std::string line;

		while(std::getline(input, line)) {
			lines.emplace_back(line);
		}
	}

	return lines;
}

int strToInt(std::string_view str) {
	
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int: " + std::string(str));

	return 0;
}

namespace Packets {
	struct IntPacketElement {
		int integer;
	};

	struct ListPacketElement;

	using Packet = std::variant<IntPacketElement, ListPacketElement>;

	struct ListPacketElement {
		std::vector<Packet> list;
	};
}

using Packets::Packet;

Packet parsePacket(std::string_view rawPacket) {
	Packets::ListPacketElement packetContainer{};

	std::stack<Packets::ListPacketElement*> parentPackets{};
	parentPackets.push(&packetContainer);

	auto push = [&parentPackets]<typename T>() mutable -> T& {
		auto& newPacket = std::get<T>(parentPackets.top()->list.emplace_back(T{}));
		return newPacket;
	};

	std::string intBuffer{};

	auto finishBuffer = [&intBuffer, &push]() mutable -> void {
		if(!intBuffer.empty()) {
			int value = strToInt(intBuffer);
			intBuffer.clear();

			push.template operator()<Packets::IntPacketElement>().integer = value;
		}
	};

	if(rawPacket.length() <= 2) {
		return packetContainer;
	}

	for(auto const& c : std::string_view(std::next(rawPacket.cbegin()), std::prev(rawPacket.cend()))) {
		switch(c) {
		case ',':
			finishBuffer();

			break;
		case '[': {
			finishBuffer();

			auto& newPacket = push.template operator()<Packets::ListPacketElement>();
			parentPackets.push(&newPacket);
			break;
		}

		case ']': 
			finishBuffer();

			parentPackets.pop();
			break;

		default:
			intBuffer.push_back(c);
			break;
		}
	}

	finishBuffer();

	return packetContainer;
}

std::vector<std::pair<Packet, Packet>> parseLines(std::vector<std::string> const& lines) {

	std::vector<std::pair<Packet, Packet>> packetPairs{};

	// last line probably does not exist and is irrelevant anyways
	for(int i = 2; i <= lines.size() ; i += 3) {
		packetPairs.emplace_back(std::make_pair(
			parsePacket(lines[i - 2]),
			parsePacket(lines[i - 1])
		));
	}

	return packetPairs;

}

struct PrintPackets {

	void operator()(Packets::IntPacketElement const& intPacket) const {
		std::cout << intPacket.integer << ",";
	}

	void operator()(Packets::ListPacketElement const& listPacket) const {
		std::cout << '[';

		for(auto const& packet : listPacket.list) {
			std::visit(PrintPackets{}, packet);
		}

		std::cout << ']';
	}
};

void printPacket(Packet const& packet) {
	std::visit(PrintPackets{}, packet);
}

// positive if left is before right
// 0 if equal
// negative if left is after right
int comparePackets(Packet const& left, Packet const& right) {
	if(std::holds_alternative<Packets::IntPacketElement>(left) && std::holds_alternative<Packets::IntPacketElement>(right)) {
		return std::get<Packets::IntPacketElement>(right).integer - std::get<Packets::IntPacketElement>(left).integer;
	}

	// at least 1 is list
	if(std::holds_alternative<Packets::ListPacketElement>(left) && std::holds_alternative<Packets::ListPacketElement>(right)) {
		auto const& leftList = std::get<Packets::ListPacketElement>(left).list;
		auto const& rightList = std::get<Packets::ListPacketElement>(right).list;
		
		for(int i = 0; i < int(std::max(leftList.size(), rightList.size())); ++i) {
			if(i >= leftList.size()) {
				return 1;
			}

			if(i >= rightList.size()) {
				return -1;
			}

			auto const comparison = comparePackets(leftList[i], rightList[i]);
			
			if(comparison != 0) {
				return comparison;
			}
		}

		return 0;
	}

	// exactly 1 is int and 1 is list
	Packets::ListPacketElement wrapper{};

	if(std::holds_alternative<Packets::IntPacketElement>(left)) {		
		wrapper.list.emplace_back(left);
		return comparePackets(wrapper, right);
	}
	else {
		wrapper.list.emplace_back(right);
		return comparePackets(left, wrapper);
	}

}

int sumCorrectPacketPairsIndices(std::vector<std::pair<Packet, Packet>> const& packetPairs) {
	int correctPairsIndicesSum = 0;

	for(int pairIdx = 1;  auto const& [leftPacket, rightPacket] : packetPairs) {
		if(comparePackets(leftPacket, rightPacket) >= 0) {
			correctPairsIndicesSum += pairIdx;

			std::cout << "Pair " << pairIdx << " is correct.\n";
		}

		++pairIdx;
	}

	return correctPairsIndicesSum;
}

Packet getDividerPacket(int value) {
	Packets::ListPacketElement outerWrap{};
	Packets::ListPacketElement innerWrap{};
	Packets::IntPacketElement valueWrap{};
	
	valueWrap.integer = value;
	innerWrap.list.emplace_back(valueWrap);
	outerWrap.list.emplace_back(innerWrap);

	return outerWrap;
}

int getDecoderKey(std::vector<std::pair<Packet, Packet>> const& packetPairs) {
	std::vector<Packet> allPackets;

	for(auto const& [left, right] : packetPairs) {
		allPackets.emplace_back(left);
		allPackets.emplace_back(right);
	}

	auto const dividerPacket1 = getDividerPacket(2);
	auto const dividerPacket2 = getDividerPacket(6);

	allPackets.emplace_back(dividerPacket1);
	allPackets.emplace_back(dividerPacket2);

	std::sort(allPackets.begin(), allPackets.end(), [](Packet const& left, Packet const& right) {
		return comparePackets(left, right) > 0;
	});

	int dividerPacket1idx = -1;
	int dividerPacket2idx = -1;

	comparePackets(dividerPacket1, dividerPacket1);

	for(int packetIdx = 1; Packet const& packet : allPackets) {
		
		if(comparePackets(packet, dividerPacket1) == 0) {
			std::cout << "Found divider packet 1 at position " << packetIdx << '\n';
			dividerPacket1idx = packetIdx;
		}

		if(comparePackets(packet, dividerPacket2) == 0) {
			std::cout << "Found divider packet 2 at position " << packetIdx << '\n';
			dividerPacket2idx = packetIdx;
		}

		++packetIdx;
	}

	return dividerPacket1idx * dividerPacket2idx;
}

int main() {

	auto inputLines = readInput("input.txt");

	auto packetPairs = parseLines(inputLines);

	auto correctPacketPairsIndicesSum = sumCorrectPacketPairsIndices(packetPairs);

	std::cout << "Correct packet pairs indices sum: " << correctPacketPairsIndicesSum << std::endl;

	auto decoderKey = getDecoderKey(packetPairs);

	std::cout << "Decoder key: " << decoderKey << std::endl;

	return 0;
}