#pragma once

#include <array>
#include <vector>

class Game {
public:
	using TShape = std::array<std::array<int, 6>, 6>;
	using TLine = std::vector<int>;
	using TBoard = std::vector<TLine>;

	Game(int width, int height) noexcept :
		Width(width),
		Height(height),
		Board(TBoard(height, TLine(width))) 
	{}
	
	void DropShape(TShape const& shape);

	void PrintBoard();

	int GetScore() {
		return Score;
	}

private:
	void InsertShapeAt(TShape const& shape, int y);

	void RemoveFullLines(int yFrom, int yTo);

	int GetFreeY(TShape const& shape);

	int Width{}, Height{};

	TBoard Board{};

	int Score{0};
};