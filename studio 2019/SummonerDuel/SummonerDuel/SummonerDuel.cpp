// SummonerDuel.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct unit_type {
	int hp;
	int damage;
	int armor;
	int mana;
	bool splash;
};

std::ostream& operator << (std::ostream& out, unit_type const& unit) {
	if (unit.damage == 0) {
		out << "M ";
	}
	out << std::max(unit.mana, unit.damage) << "/" << unit.hp;
	if (unit.armor > 0) {
		out << "a" << unit.armor;
	}
	if (unit.splash) {
		out << "s";
	}
	return out;
}

struct game_type;

struct player_type {
	std::string name;
	unit_type mage;
	std::vector<unit_type> creatures;
	int mover_index = -2;

	unit_type* mover() {
		if (mover_index == -2) {
			return nullptr;
		}
		if (mover_index == -1) {
			return &mage;
		}
		return &creatures[mover_index];
	}

	player_type() {
		mage.hp = 12;
	}

	void start_move(bool first_move = false) {
		mage.mana += first_move ? 1 : 2;
		mover_index = -1;
	}

	void print(std::ostream& out, game_type& game) {
		out << fmt::format(is_mover(game) ? "[{0}]" : "{0}", name) 
			<< " - " << mage << "\n";
		for (auto c : creatures) {
			out << c << "\n";
		}
	}

	bool is_mover(game_type const& game) const;
};



struct game_type {
	player_type players[2];
	int mover_index;
	int move_number;

	const player_type& mover() const {
		assert(mover_index < 2);
		return players[mover_index];
	}

	game_type() {
		players[0].name = "player1";
		players[1].name = "player2";
	}

	std::string command_prompt() {
		return "What to do?";
	}

	void print(std::ostream& out) {
		out << "Move #" << game.move_number << "\n\n";
		players[0].print(out, *this);
		out << "\n";
		players[1].print(out, *this);
	}
} game;

bool player_type::is_mover(game_type const& game) const {
	return this == std::addressof(game.mover());
}

std::string command;

void clear() {
	system("cls");
}

int _tmain(int argc, _TCHAR* argv[])
{
	game.players[0].start_move(true);

	while (true) {
		clear();
		game.print(std::cout);
		std::cout << "\n";
		std::cout << game.command_prompt() << " > ";
		getline(std::cin, command);
		if (command.size() == 0) {
			continue;
		}
		if (command[0] == 'q') {
			break;
		}
	}
}

