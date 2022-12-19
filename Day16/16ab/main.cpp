#include <bitset>
#include <charconv>
#include <deque>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <omp.h>

void crash(std::string const& message) {
	std::cerr << message << std::endl;
	std::terminate();
}

struct Valve {
	int id{};
	int flow{};
	std::vector<int> neighbors;
	bool deleted{ false };
};

int strToInt(std::string const& str) {
	[[likely]]
	if(int result; std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	crash("Could not parse int from " + str);
	return 0;
}

static std::unordered_map<std::string, int> ids;
static std::unordered_map<int, std::string> names;

int getVertexId(std::string const& name) {
	static int id = 0;

	if(ids.count(name) > 0) {
		return ids[name];
	}

	auto const myId = id++;

	names[myId] = name;
	return ids[name] = myId;
}

std::string const& getVertexName(int vertexId) {
	return names[vertexId];
}

std::vector<Valve> parseLines(std::vector<std::string> const& lines) {
	std::vector<Valve> valves;
	valves.reserve(lines.size());

	std::regex pattern("Valve ([A-Z]{2}) has flow rate=([0-9]+); tunnels? leads? to valves? ([A-Z, ]+)");

	std::smatch match;

	for(auto const& line : lines) {
		[[unlikely]]
		if(!std::regex_search(line, match, pattern)) {
			crash("Could not match regex.");
		}

		[[unlikely]]
		if(match.size() != 4) {
			crash("Got unexpected match group count.");
		}

		std::string const valveName = match[1];
		std::string const flowRateRaw = match[2];
		std::string const neighborsRaw = match[3];

		int const valveId = getVertexId(valveName);
		int const flowRate = strToInt(flowRateRaw);

		std::vector<int> neighbors;

		for(auto const valveNameRange : neighborsRaw | std::views::split(std::string_view(", "))) {
			std::string valveName(valveNameRange.begin(), valveNameRange.end());

			int const valveId = getVertexId(valveName);

			neighbors.emplace_back(valveId);
		}

		valves.emplace_back(Valve{ .id = valveId, .flow = flowRate, .neighbors = std::move(neighbors) });
	}

	return valves;
}

std::vector<std::string> readLines(std::string const& fileName) {
	std::ifstream input(fileName);

	[[unlikely]]
	if(!input.is_open()) {
		crash("Could not open the input file");
	}


	std::vector<std::string> lines;

	{
		std::string line;

		while(std::getline(input, line)) {
			lines.emplace_back(line);
		}
	}

	return lines;
	
}

constexpr
int64_t getMaxMask(int64_t bits) {
	return (int64_t(1) << bits) - 1;
}

constexpr
int64_t selector(int64_t position) {
	return int64_t(1) << position;
}

constexpr
bool isBitSet(int64_t position, int64_t mask) {
	return (selector(position) & mask) != 0;
}

constexpr
int64_t setBit(int64_t position, int64_t mask) {
	return selector(position) | mask;
}

constexpr
int64_t negateMask(int64_t mask, int64_t maxMask) {
	return (~mask) & maxMask;
}

class Graph {
	static int inf;
public:
	Graph(std::vector<Valve> const& valves) noexcept {
		Valves.reserve(valves.size());

		for(auto const& valve : valves) {
			int const valveId = valve.id;
			Valves.emplace(valveId, valve);

			for(auto const& valve2 : valves) {
				Neighbors[valveId].emplace(valve2.id, inf);
			}

			for(auto const& neighbor : valve.neighbors) {
				Neighbors[valveId][neighbor] = 1;
			}
		}
	}

	void MakeShortestPaths() {
		for(auto const& [source, _] : Valves) {
			for(int i = 0; i < int(Valves.size()); ++i) {
				for(auto& [v1, neighbors] : Neighbors) {
					for(auto& [v2, distance] : neighbors) {

						int const distanceToV1 = Neighbors[source][v1];
						int& distanceToV2 = Neighbors[source][v2];

						distanceToV2 = std::min(distanceToV2, distanceToV1 + distance);
					}
				}
			}
		}

		for(auto& [source, neighbors] : Neighbors) {
			neighbors.erase(source);
		}
	}

	void RemoveVerticesHardIf(auto const& pred) {
		for(auto& [id, valve] : Valves) {
			if(pred(valve)) {
				RemoveVertex(id);
			}
		}
	}

	// [TIME][ACTIVE MASK][VERTEX] -> FLOW
	std::vector<std::vector<std::vector<int>>> MakeLookUpTable(int maxTime) const {
		std::vector<int> vertexMap;

		for(auto const& [id, valve] : Valves) {
			if(!valve.deleted) {
				vertexMap.emplace_back(id);
			}
		}

		int const nVertices = int(vertexMap.size());

		std::vector<std::vector<std::vector<int>>> lookUpTable(
			maxTime + 1, std::vector<std::vector<int>>(
				getMaxMask(nVertices) + 1LL, std::vector<int>(
					nVertices, 0
				)
			)
		);

		for(int time = 1; time <= maxTime; ++time) {
			
#pragma omp parallel for
			for(int64_t mask = 0; mask <= getMaxMask(nVertices); ++mask) {
				
				for(int source = 0; source < nVertices; ++source) {

					int const mappedSource = vertexMap[source];
					auto const sourceFlow = Valves.at(mappedSource).flow;
					int const myFlow = (time - 1) * sourceFlow;
					int bestExtraFlow = 0;
					auto const newMask = setBit(source, mask);
				
					for(int neighbor = 0; neighbor < nVertices; ++neighbor) {
						
						// also skips if neighbor == source
						if(isBitSet(neighbor, newMask)) {
							continue;
						}

						int const mappedNeighbor = vertexMap[neighbor];
						int const distance = Neighbors.at(mappedSource).at(mappedNeighbor);

						if(time - distance < 1) {
							continue;
						}

						bestExtraFlow = std::max(bestExtraFlow, lookUpTable[time - distance - 1][newMask][neighbor]);

					}

					lookUpTable[time][mask][source] = myFlow + bestExtraFlow;

				}

			} // end of parallel for

		}

		return lookUpTable;
	}

	void RemoveVertex(int id) {
		Valves[id].deleted = true;

		for(auto& [neighbor, distance] : Neighbors[id]) {
			if(Neighbors[neighbor].count(id) > 0) {
				Neighbors[neighbor].erase(id);
			}
		}
	}

	std::unordered_map<int, Valve> const& GetValves() const {
		return Valves;
	}

	auto const& GetNeighbors() const {
		return Neighbors;
	}

private:
	std::unordered_map<int, std::unordered_map<int, int>> Neighbors;
	std::unordered_map<int, Valve> Valves;
};


int Graph::inf = 9999;

int main() {
	
	auto lines = readLines("input.txt");

	auto parsedValves = parseLines(lines);

	Graph graph(parsedValves);

	graph.MakeShortestPaths();

	graph.RemoveVerticesHardIf([](Valve const& valve) -> bool {return valve.flow == 0; });

	auto const& valves = graph.GetValves();
	auto const& aasNeighbors = graph.GetNeighbors().at(getVertexId("AA"));

	auto lookUpTable1 = graph.MakeLookUpTable(30);

	int result1 = 0;

	for(int mappedId = 0;  auto const& [id, valve] : valves) {
		if(valve.deleted) {
			continue;
		}

		int const distance = aasNeighbors.at(valve.id);

		result1 = std::max(result1, lookUpTable1[30 - distance][0][mappedId]);

		++mappedId;
	}

	std::cout << result1 << '\n';

	int nValves = 0;

	for(auto const& [id, valve] : valves) {
		if(!valve.deleted) {
			++nValves;
		}
	}

	int result2 = 0;

	auto lookUpTable2 = graph.MakeLookUpTable(26);

	auto const maxMask = getMaxMask(nValves);

#pragma omp parallel for
	for(int64_t mask = 0; mask <= maxMask; ++mask) {

		auto const negatedMask = negateMask(mask, maxMask);
		
		for(int mappedNeighbor1id = 0; auto const& [id1, valve1] : valves) {
			if(valve1.deleted) {
				continue;
			}

			if(!isBitSet(mappedNeighbor1id, mask)) {

				int const distance1 = aasNeighbors.at(valve1.id);

				int const result2_me = lookUpTable2[26 - distance1][mask][mappedNeighbor1id];

				for(int mappedNeighbor2id = 0; auto const& [id2, valve2] : valves) {
					if(valve2.deleted) {
						continue;
					}

					if(!isBitSet(mappedNeighbor2id, negatedMask)) {

						int const distance2 = aasNeighbors.at(valve2.id);

						int const result2_elephant = lookUpTable2[26 - distance2][negatedMask][mappedNeighbor2id];

#pragma omp critical
						result2 = std::max(result2, result2_me + result2_elephant);

					}

					++mappedNeighbor2id;
				}
			}

			++mappedNeighbor1id;
		}

	}

	std::cout << result2 << '\n';

	return 0;

}