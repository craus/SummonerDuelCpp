// SummonerDuel.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct game_type;
struct player_type;

struct unit_type {
	int hp = 1;
	int damage = 1;
	int armor = 0;
	int mana = 0;
	bool splash = false;

	bool dead() const {
		return hp <= 0;
	}

	bool is_mover(player_type const& player) const;

	unit_type() {}

	unit_type(int damage, int hp, int armor, bool splash)
		: damage(damage), hp(hp), armor(armor), splash(splash) {
	}

	int manacost() {
		return hp / 2 + damage + armor + (splash ? 2 : 0);
	}

	bool correct() {
		return hp > 0 && damage > 0 && armor >= 0;
	}

	void print(std::ostream& out, player_type& owner) {
		out << fmt::format(is_mover(owner) ? "[{0}]" : "{0}", *this);
	}

	void move(player_type& owner, game_type& game);
	void hit(unit_type& attacker, game_type& game);
};

std::ostream& operator <<(std::ostream& out, unit_type unit) {
	out << fmt::format("{0}", unit);
	return out;
}

namespace fmt {
	template <>
	struct formatter<unit_type> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const unit_type& unit, FormatContext& ctx) {
			return format_to(
				ctx.begin(),
				"{0}{1}/{2}{3}{4}",
				unit.damage == 0 ? "M " : "",
				std::max(unit.mana, unit.damage),
				unit.hp,
				unit.armor > 0 ? fmt::format("a{0}", unit.armor) : "",
				unit.splash ? "s" : ""
			);
		}
	};
}

struct player_type {
	std::string name = "UNKNOWN PLAYER";
	unit_type mage;
	std::vector<unit_type> creatures;
	int mover_index = -2;

	const unit_type* mover() const {
		if (mover_index == -2) {
			return nullptr;
		}
		if (mover_index == -1) {
			return &mage;
		}
		return &creatures[mover_index];
	}
	unit_type* mover() {
		if (mover_index == -2) {
			return nullptr;
		}
		if (mover_index == -1) {
			return &mage;
		}
		return &creatures[mover_index];
	}

	bool mage_move() {
		return mover() == &mage;
	}

	player_type() {
		mage.hp = 12;
	}

	void start_move(bool first_move = false) {
		mage.mana += first_move ? 1 : 2;
		if (creatures.size() > 0) {
			mover_index = 0;
		} else {
			mover_index = -1;
		}
	}

	void remove_dead_creatures() {
		creatures.erase(
			std::remove_if(
				creatures.begin(),
				creatures.end(),
				[](const unit_type& u) { return u.dead(); }
			),
			creatures.end()
		);
	}

	bool is_mover(game_type const& game) const;
	void end_turn(game_type & game);	
	void summon_creature(game_type& game, int dmg, int hp, int armour, bool splash);
	void attack(game_type& game, int target);

	void next_unit(); 
	void skip(game_type& game);

	void print(std::ostream& out, game_type& game);
};

bool unit_type::is_mover(player_type const& player) const {
	return this == player.mover();
}

struct game_type {
	static const int P = 2;
	player_type players[P];
	int mover_index = 0;
	int move_number = 0;

	const player_type& mover() const {
		assert(mover_index < P);
		return players[mover_index];
	}
	player_type& mover() {
		assert(mover_index < P);
		return players[mover_index];
	}

	player_type& enemy(player_type& player) {
		if (&player == &players[0]) {
			return players[1];
		}
		return players[0];
	}

	game_type() {
		players[0].name = "player1";
		players[1].name = "player2";
	}

	std::string command_prompt() {
		if (mover().mage_move()) {
			return "What to do?";
		}
		return "Who to attack?";
	}

	void end_turn() {
		move_number++;
		mover_index = (mover_index + 1) % P;
		mover().start_move();
	}

	void print(std::ostream& out) {
		out << "Move #" << game.move_number << "\n\n";
		players[0].print(out, *this);
		out << "\n";
		players[1].print(out, *this);
	}

	void after_unit_hit() {
		for (auto& p : players) {
			if (p.mage.dead()) {
				win(enemy(p));
				return;
			}
			p.remove_dead_creatures();
		}
	}

	void win(player_type& p) {
		std::cout << fmt::format("{0} wins!", p.name);
	}
} game;

void player_type::print(std::ostream& out, game_type& game) {
	out << fmt::format(is_mover(game) ? "[{0}]" : "{0}", name)
		<< " - ";
	mage.print(out, *this);
	out << "\n";
	out << "mover_index: " << mover_index << "\n";
	int index = 1;
	for (auto& c : creatures) {
		if (game.enemy(*this).is_mover(game) && !game.mover().mage_move()) {
			out << fmt::format("<{0}> ", index);
			index++;
		}
		c.print(out, *this);
		out << "\n";
	}
}

bool player_type::is_mover(game_type const& game) const {
	return this == std::addressof(game.mover());
}

void player_type::end_turn(game_type &game) {
	std::cout << "Finishing turn...\n";
	mover_index = -2;
	game.end_turn();
}

void player_type::summon_creature(game_type &game, int dmg, int hp, int armour, bool splash) {
	unit_type u(dmg, hp, armour, splash);
	int m = u.manacost();
	std::cout << fmt::format("Summoning creature: {0} (manacost {1})\n", u, m);
	if (!mage_move()) {
		std::cout << fmt::format("Cannot summon creature. It's not mage's move\n");
		return;
	}
	if (!u.correct()) {
		std::cout << fmt::format("Incorrect creature parameters\n");
		return;
	}
	if (mage.mana < m) {
		std::cout << fmt::format("Not enough mana ({0}/{1})\n", m, mage.mana);
		return;
	}
	mage.mana -= m;
	creatures.push_back(u);

	if (mage.mana == 0) {
		end_turn(game);
	}
}

void player_type::attack(game_type& game, int targetIndex) {
	if (mage_move()) {
		std::cout << fmt::format("Cannot attack. It's mage's move\n");
		return;
	}
	player_type & enemy = game.enemy(*this);
	if (targetIndex > enemy.creatures.size()) {
		std::cout << fmt::format("Cannot attack. No such target exists\n");
		return;
	}
	unit_type * target = 
		targetIndex == 0 
		? &enemy.mage 
		: &enemy.creatures[targetIndex-1];
	target->hit(*mover(), game);
	next_unit();
}

void player_type::skip(game_type & game) {
	std::cout << fmt::format("{0} finishes his move\n", name);
	game.end_turn();
}

void player_type::next_unit() {
	mover_index++;
	if (mover_index == creatures.size()) {
		mover_index = -1;
	} else {
		mover()->move(*this, game);
	}
}

void unit_type::move(player_type& owner, game_type& game) {
	if (!splash) {
		return;
	}
	for (auto& c : game.enemy(owner).creatures) {
		c.hit(*this, game);
	}
	game.enemy(owner).mage.hit(*this, game);
}

void unit_type::hit(unit_type& attacker, game_type& game) {
	int dmg = std::max(0, attacker.damage - armor);
	std::cout << fmt::format("{0} attacks {1} for {2} dmg\n", attacker, *this, dmg);
	if (hp - dmg <= 0) {
		std::cout << fmt::format("{0} dies\n", *this);
	}
	hp -= dmg;
	game.after_unit_hit();
}

std::string command;

void clear() {
	system("cls");
}

void new_game() {
	game = game_type();
	game.players[0].start_move(true);
}

int _tmain(int argc, _TCHAR* argv[])
{
	new_game();

	while (true) {
		game.print(std::cout);
		std::cout << "\n";
		std::cout << game.command_prompt() << " > ";
		getline(std::cin, command);
		clear();
		std::cout << "> " << command << "\n";
		std::istringstream ss(command);

		if (command[0] == 'q') {
			break;
		} else if (command[0] == 'r') {
			std::cout << "Restarting game..." << "\n";
			new_game();
		} else if (command[0] == 'c') {
			std::string c;
			int dmg = 1, hp = 1, armour = 0;
			std::string splash = "no";
			ss >> c;
			if (!ss.eof()) ss >> dmg;
			if (!ss.eof()) ss >> hp;
			if (!ss.eof()) ss >> armour;
			if (!ss.eof()) ss >> splash;
			game.mover().summon_creature(game, dmg, hp, armour, splash == "s");
		} else {
			if (game.mover().mage_move()) {
				if (command == "") {
					game.mover().skip(game);
				} else {
					std::cout << "Cannot recognize command\n";
				}
			} else {
				int target = 0;
				ss >> target;
				game.mover().attack(game, target);
			}
		}

		std::cout << "\n";
	}
}

