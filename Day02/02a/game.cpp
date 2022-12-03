#include "game.h"

#include <iostream>
#include <io.h>
#include <fcntl.h>

void Game::DropShape(TShape const& shape) {
	
	int const freeY = GetFreeY(shape);
	
	InsertShapeAt(shape, freeY);
}


int Game::GetFreeY(TShape const& shape) {
	 _setmode(_fileno(stdout), _O_U16TEXT);

	int const shapeHeight = int(shape.size());
	int const shapeWidth = int(shape.front().size());

	for(int y = this->Height - 1; y >= shapeHeight - 1; --y) {
		for(int yOff = 0; yOff < shapeHeight; ++yOff) {
			for(int x = 0; x < shapeWidth; ++x) {
				if(Board[y - yOff][x] == 1 && shape[yOff][x] == 1) {
					return y + 1;
				}
			}
		}
	}

	return shapeHeight - 1;
}

void Game::InsertShapeAt(TShape const& shape, int y) {
	for(int yOff = 0; yOff < shape.size(); ++yOff) {
		for(int xOff = 0; xOff < shape.front().size(); ++xOff) {
			Board[y - yOff][xOff] |= shape[yOff][xOff];
		}
	}

	RemoveFullLines(y - int(shape.size()) + 1, y + 1);
}

void Game::RemoveFullLines(int yFrom, int yTo) {
	for(int y = yFrom; y < yTo; ++y) {
		for(auto item : Board[y]) {
			if(item == 0) {
				goto ContinueOuter;
			}
		}

		Board.erase(std::next(Board.begin(), y));
		Board.emplace_back(this->Width);
		--y;
		++Score;

		ContinueOuter: {}
	}
}

void Game::PrintBoard() {
	std::wcout << L"\n";

	int y = this->Height - 1;

	for(; y >= 0; --y) {
		for(int x = 0; x < this->Width; ++x) {
			if(Board[y][x]) {
				goto BreakOuter;
			}
		}
	}

BreakOuter: {}

	for(; y >= 0; --y) {
		for(int x = 0; x < this->Width; ++x) {
			std::wcout << (Board[y][x] ? L'◘' : L'_');
		}

		std::wcout << '\n';
	}

	std::wcout << "--- Score: " << this->Score << '\n';
}