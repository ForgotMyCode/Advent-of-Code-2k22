// NOTE:
// THIS APPROACH IS INTENTIONALLY ESOTERIC AS A PART OF THE CHALLENGE

#include "game.h"

#include <cassert>
#include <fstream>
#include <string>
#include <iostream>
#include <unordered_map>

constexpr Game::TShape cookShape(int a, int b, int c, int d, int e, int f) {
	return Game::TShape{
		{
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
		}
	};
}

constexpr Game::TShape cookHalfShape(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l) {
	return Game::TShape{
		{
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{a, b, c, d, e, f},
			{g, h, i, j, k, l},
			{g, h, i, j, k, l},
			{g, h, i, j, k, l},
		}
	};
}

constexpr Game::TShape cookLines(int n) {
	Game::TShape shape{};

	for(int y = 0; y < n; ++y) {
		for(int x = 0; x < shape.front().size(); ++x) {
			shape[y][x] = 1;
		}
	}

	return shape;
}

int main() {
	Game game(6, 45100);

	static constexpr auto enemyLine = cookShape(1, 0, 0, 0, 0, 0);
	static constexpr auto enemySquareWidth2 = cookShape(1, 1, 0, 0, 0, 0);
	static constexpr auto enemySquareWidth3 = cookShape(1, 1, 1, 0, 0, 0);
	static constexpr auto enemySquareWidth4 = cookShape(1, 1, 1, 1, 0, 0);
	static constexpr auto meLine = cookShape(0, 0, 0, 0, 0, 1);
	static constexpr auto meHalfSquareWidth2 = cookHalfShape(0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1);
	static constexpr auto meHalfSquareWidth3 = cookHalfShape(0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1);
	static constexpr auto meHalfSquareWidth4 = cookHalfShape(0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1);
	static constexpr auto meSquareWidth2 = cookShape(0, 0, 0, 0, 1, 1);
	static constexpr auto meSquareWidth3 = cookShape(0, 0, 0, 1, 1, 1);
	static constexpr auto meSquareWidth4 = cookShape(0, 0, 1, 1, 1, 1);
	static constexpr auto oneImmediateLine = cookLines(1);
	static constexpr auto twoImmediateLines = cookLines(2);
	static constexpr auto threeImmediateLines = cookLines(3);

	std::ifstream input;
	input.open("input.txt");

	std::string line;

	int turn = 0;

	int expected = 0;
	
	std::vector<std::vector<int>> table{
		{4, 8, 3},
		{1, 5, 9},
		{7, 2, 6}
	};
	
	while(std::getline(input, line)) {
		char const opponent = line[0];
		char const me = line[2];

		int oppI = opponent - 'A';
		int meI = me - 'X';

		expected += table[oppI][meI];

		switch(opponent) {
		case 'A':
			game.DropShape(enemySquareWidth4);
			game.DropShape(enemyLine);
			game.DropShape(enemyLine);
			break;
		case 'B':
			game.DropShape(enemyLine);
			game.DropShape(enemySquareWidth3);
			game.DropShape(enemyLine);
			break;
		case 'C':
			game.DropShape(enemyLine);
			game.DropShape(enemyLine);
			game.DropShape(enemySquareWidth2);
			break;
		default:
			exit(1);
		}

		switch(me) {
		case 'X':
			game.DropShape(meHalfSquareWidth2);
			game.DropShape(meLine);
			game.DropShape(meSquareWidth4);
			game.DropShape(oneImmediateLine);
			break;
		case 'Y':
			game.DropShape(meSquareWidth2);
			game.DropShape(meHalfSquareWidth3);
			game.DropShape(meLine);
			game.DropShape(twoImmediateLines);
			break;
		case 'Z':
			game.DropShape(meLine);
			game.DropShape(meSquareWidth3);
			game.DropShape(meHalfSquareWidth4);
			game.DropShape(threeImmediateLines);
			break;
		default:
			exit(2);
		}


		++turn;

		assert(expected == game.GetScore());

		std::wcout << "Turn " << turn << " Score: " << game.GetScore() << '\n';
	}
	game.PrintBoard();

	std::wcout << "DONE!" << std::endl;

	return 0;
}