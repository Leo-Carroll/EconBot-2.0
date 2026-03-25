#include <dpp/dpp.h>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <atomic>

static int GetRandomInt(int min, int max) {
	static std::mt19937 gen(std::random_device{}());
	return std::uniform_int_distribution<int>(min, max)(gen);
}

struct Job {
	std::string title;
	int payPerHour;
	int minHours;
	int maxHours;
	int promotionThreshold;

	Job(const std::string& title, int payPerHour, int minHours, int maxHours, int promotionThreshold)
		: title(title), payPerHour(payPerHour),
		  minHours(minHours), maxHours(maxHours),
		  promotionThreshold(promotionThreshold){}
};

const std::vector<std::vector<Job>> JOB_PATHS = {
	// Path 0: Food Service — low pay, low threshold. Total works to max: ~50
	{
		Job("Fast Food Worker",   2, 1, 3,  5),
		Job("Line Cook",          4, 2, 3,  8),
		Job("Sous Chef",          6, 2, 4, 12),
		Job("Head Chef",         10, 3, 4, 15),
		Job("Restaurant Owner",  16, 3, 5, 10),
	},
	// Path 1: Retail — low-mid pay, low-mid threshold. Total works to max: ~50
	{
		Job("Cashier",            3, 1, 3,  5),
		Job("Stock Associate",    5, 2, 3,  8),
		Job("Department Manager", 8, 2, 4, 12),
		Job("Store Manager",     13, 3, 4, 15),
		Job("Regional Director", 20, 3, 5, 10),
	},
	// Path 2: Technology — high pay, high threshold. Total works to max: ~50
	{
		Job("IT Helpdesk",        6, 2, 3,  5),
		Job("Junior Developer",  10, 2, 4,  8),
		Job("Software Engineer", 16, 3, 4, 12),
		Job("Senior Engineer",   24, 3, 5, 15),
		Job("Engineering Lead",  36, 3, 5, 10),
		Job("CTO", 50, 4, 5, 10),
	},
	// Path 3: Finance — high pay, high threshold. Total works to max: ~50
	{
		Job("Bank Teller",        6, 1, 3,  5),
		Job("Loan Officer",      10, 2, 3,  8),
		Job("Financial Analyst", 16, 2, 4, 12),
		Job("Portfolio Manager", 25, 3, 4, 15),
		Job("Investment Director",38,3, 5, 10),
	},
	// Path 4: Law Enforcement — mid pay, mid threshold. Total works to max: ~50
	{
		Job("Security Guard",     4, 1, 3,  5),
		Job("Police Officer",     8, 2, 3,  8),
		Job("Detective",         13, 2, 4, 12),
		Job("Sergeant",          20, 3, 4, 15),
		Job("Police Chief",      30, 3, 5, 10),
	},
	// Path 5: Healthcare — high pay, high threshold. Total works to max: ~50
	{
		Job("Hospital Orderly",   5, 1, 3,  5),
		Job("Nurse",             10, 2, 4,  8),
		Job("Physician",         18, 2, 4, 12),
		Job("Specialist",        28, 3, 5, 15),
		Job("Chief of Medicine", 42, 3, 5, 10),
	},
	// Path 6: Entertainment — mid pay, mid threshold. Total works to max: ~50
	{
		Job("Street Performer",   3, 1, 2,  5),
		Job("Local Band Member",  5, 1, 3,  8),
		Job("Studio Musician",    9, 2, 4, 12),
		Job("Recording Artist",  16, 2, 4, 15),
		Job("Music Producer",    26, 3, 5, 10),
	},
};

constexpr size_t SECONDS_IN_HALF_HOUR = 30 * 60;
constexpr size_t SECONDS_IN_HOUR = SECONDS_IN_HALF_HOUR * 2;
constexpr size_t SECONDS_IN_DAY = 24 * 60 * 60;
constexpr size_t SECONDS_IN_WEEK = 7 * 24 * 60 * 60;

const std::unordered_set<std::string> ADMIN_IDS = {
	"338844132904534048"
};

static std::vector<uint8_t> MakeDeck() {
	std::vector<uint8_t> deck;
	deck.reserve(52);
	for (int suit = 0; suit < 4; ++suit) {
		for (int rank = 1; rank <= 13; ++rank) {
			deck.push_back(static_cast<uint8_t>(rank));
		}
	}
	static std::mt19937 rng(std::random_device{}());
	std::shuffle(deck.begin(), deck.end(), rng);
	return deck;
}

static int CardValue(uint8_t rank) {
	if (rank >= 10) return 10;
	return rank;
}

static int HandTotal(const std::vector<uint8_t>& hand) {
	int total = 0;
	int aces = 0;
	for (uint8_t card : hand) {
		if (card == 1) {
			aces++;
			total += 11;
		}
		else {
			total += CardValue(card);
		}
	}
	while (total > 21 && aces > 0) {
		total -= 10;
		--aces;
	}
	return total;
}

static std::string CardName(uint8_t rank) {
	switch (rank) {
	case 1:  return "A";
	case 11: return "J";
	case 12: return "Q";
	case 13: return "K";
	default: return std::to_string(rank);
	}
}

static std::string HandString(const std::vector<uint8_t>& hand) {
	std::string result;
	for (size_t i = 0; i < hand.size(); ++i) {
		if (i > 0) result += ", ";
		result += CardName(hand[i]);
	}
	result += std::format(" ({})", HandTotal(hand));
	return result;
}

struct BlackjackGame {
	std::vector<uint8_t> playerHand;
	std::vector<uint8_t> dealerHand;
	std::vector<uint8_t> deck;
	size_t bet;
	bool finished;

	BlackjackGame()
		: bet(0), finished(true) {}

	BlackjackGame(size_t bet)
		: bet(bet), finished(false) {

		deck = MakeDeck();

		playerHand.push_back(deck.back()); 
		deck.pop_back();
		dealerHand.push_back(deck.back());
		deck.pop_back();
		playerHand.push_back(deck.back());
		deck.pop_back();
		dealerHand.push_back(deck.back());
		deck.pop_back();
	}

	uint8_t draw_card() {
		uint8_t card = deck.back();
		deck.pop_back();
		return card;
	}
};

struct RouletteGame {
	size_t bet;
	std::string choice;
	bool finished;

	RouletteGame()
		: bet(0), choice(""), finished(true) {}

	RouletteGame(size_t bet, const std::string& choice)
		: bet(bet), choice(choice), finished(false) {}
};

constexpr size_t SOFT_CAP = 100000;

static size_t ApplyDiminishingReturns(size_t currentTotal, size_t amount) {
	if (currentTotal >= SOFT_CAP) {
		return 1;
	}
	double ratio = 1.0 - (static_cast<double>(currentTotal) / static_cast<double>(SOFT_CAP));
	double scaled = static_cast<double>(amount) * ratio * ratio;
	return std::max(static_cast<size_t>(1), static_cast<size_t>(scaled));
}

constexpr bool DO_DIMINISHING_RETURNS = true;

constexpr size_t DUEL_EXPIRY_SECONDS = SECONDS_IN_HOUR;

struct PendingDuel {
	std::string challengerId;
	size_t bet;
	size_t expiresAt;

	PendingDuel() 
		: challengerId(""), bet(0), expiresAt(static_cast<size_t>(time(nullptr))) {}

	PendingDuel(const std::string& challenger, size_t bet)
		: challengerId(challenger), bet(bet), expiresAt(static_cast<size_t>(time(nullptr) + DUEL_EXPIRY_SECONDS)) {}
};

struct User {
	std::string id;
	size_t cash;
	size_t bank;

	bool hasJob;
	int jobPathIdx;
	int jobIdx;
	size_t lastWork;
	int timesWorked;

	size_t lastDaily;
	int dailyStreak;
	size_t lastWeekly;
	int weeklyStreak;

	bool activeCoinflip;

	bool activeBlackjack;
	BlackjackGame blackjackGame;

	bool activeRoulette;
	RouletteGame rouletteGame;

	size_t lastRobbery;

	size_t lastPassive;
	size_t passiveBalance;

	User(const std::string& id)
		: id(id), cash(0), bank(0),
		  hasJob(false), jobPathIdx(0), jobIdx(0),
		  lastWork(time(nullptr) - SECONDS_IN_HALF_HOUR), timesWorked(0),
	      lastDaily(time(nullptr) - SECONDS_IN_DAY), dailyStreak(0),
		  lastWeekly(time(nullptr) - SECONDS_IN_WEEK), weeklyStreak(0),
		  activeCoinflip(false), 
		  activeBlackjack(false), blackjackGame(),
		  activeRoulette(false), rouletteGame(),
		  lastRobbery(time(nullptr) - SECONDS_IN_HOUR),
		  lastPassive(static_cast<size_t>(time(nullptr))), passiveBalance(0) {}
};

void SaveUsers(const std::vector<User>& users) {
	nlohmann::json root;

	for (const auto& user : users) {
		nlohmann::json curr;

		curr["id"] = user.id;
		curr["cash"] = user.cash;
		curr["bank"] = user.bank;

		curr["has_job"] = user.hasJob;
		curr["job_path_idx"] = user.jobPathIdx;
		curr["job_idx"] = user.jobIdx;
		curr["last_work"] = user.lastWork;
		curr["times_worked"] = user.timesWorked;

		curr["last_daily"] = user.lastDaily;
		curr["daily_streak"] = user.dailyStreak;
		curr["last_weekly"] = user.lastWeekly;
		curr["weekly_streak"] = user.weeklyStreak;

		curr["active_blackjack"] = user.activeBlackjack;

		if (user.activeBlackjack) {
			nlohmann::json blackjackGame;
			blackjackGame["player_hand"] = user.blackjackGame.playerHand;
			blackjackGame["dealer_hand"] = user.blackjackGame.dealerHand;
			blackjackGame["deck"] = user.blackjackGame.deck;
			blackjackGame["bet"] = user.blackjackGame.bet;

			curr["blackjack_game"] = blackjackGame;
		}

		curr["active_roulette"] = user.activeRoulette;
		if (user.activeRoulette) {
			nlohmann::json rouletteGame;
			rouletteGame["bet"] = user.rouletteGame.bet;
			rouletteGame["choice"] = user.rouletteGame.choice;

			curr["roulette_game"] = rouletteGame;
		}

		curr["last_robbery"] = user.lastRobbery;

		curr["last_passive"] = user.lastPassive;
		curr["passive_balance"] = user.passiveBalance;

		root[user.id] = curr;
	}

	std::ofstream file("UserData.json");
	file << root.dump(4);
	file.close();
}

void LoadUsers(std::vector<User>& users) {
	std::ifstream file("UserData.json");
	if (!file.is_open()) {
		return;
	}

	nlohmann::json userJson;

	try {
		file >> userJson;
	}
	catch (...) {
		return;
	}

	for (auto it = userJson.begin(); it != userJson.end(); ++it) {
		const auto& curr = it.value();

		std::string id = curr.value("id", it.key());
		User user(id);

		user.cash = curr.value("cash", static_cast<size_t>(0));
		user.bank = curr.value("bank", static_cast<size_t>(0));

		user.hasJob = curr.value("has_job", false);
		user.jobPathIdx = curr.value("job_path_idx", 0);
		user.jobIdx = curr.value("job_idx", 0);
		user.lastWork = curr.value("last_work", static_cast<size_t>(time(nullptr) - SECONDS_IN_HALF_HOUR));
		user.timesWorked = curr.value("times_worked", 0);

		user.lastDaily = curr.value("last_daily", static_cast<size_t>(time(nullptr) - SECONDS_IN_DAY));
		user.dailyStreak = curr.value("daily_streak", 0);
		user.lastWeekly = curr.value("last_weekly", static_cast<size_t>(time(nullptr) - SECONDS_IN_WEEK));
		user.weeklyStreak = curr.value("weekly_streak", 0);
		user.activeBlackjack = curr.value("active_blackjack", false);

		if (user.activeBlackjack && curr.contains("blackjack_game")) {
			nlohmann::json blackjack = curr["blackjack_game"];
			user.blackjackGame.playerHand = blackjack.value("player_hand", std::vector<uint8_t>{});
			user.blackjackGame.dealerHand = blackjack.value("dealer_hand", std::vector<uint8_t>{});
			user.blackjackGame.deck = blackjack.value("deck", std::vector<uint8_t>{});
			user.blackjackGame.bet = blackjack.value("bet", static_cast<size_t>(0));
			user.blackjackGame.finished = false;
		}

		user.activeRoulette = curr.value("active_roulette", false);

		if (user.activeRoulette && curr.contains("roulette_game")) {
			nlohmann::json roulette = curr["roulette_game"];
			user.rouletteGame.bet = roulette.value("bet", 0);
			user.rouletteGame.choice = roulette.value("choice", "0");
			user.rouletteGame.finished = false;
		}

		user.lastRobbery = curr.value("last_robbery", static_cast<size_t>(time(nullptr) - SECONDS_IN_HOUR));
		user.lastPassive = curr.value("last_passive", static_cast<size_t>(time(nullptr)));
		user.passiveBalance = curr.value("passive_balance", static_cast<size_t>(0));

		users.push_back(user);
	}
}

constexpr double PASSIVE_RATE_PER_HOUR = 0.01;
constexpr size_t PASSIVE_MAX_HOURS = 6;

static size_t CalcPassiveIncome(size_t bank, size_t lastPassive) {
	size_t currentTime = static_cast<size_t>(time(nullptr));
	if (currentTime <= lastPassive) {
		return 0;
	}

	double hoursElapsed = static_cast<double>(currentTime - lastPassive) / static_cast<double>(SECONDS_IN_HOUR);
	hoursElapsed = std::min(hoursElapsed, static_cast<double>(PASSIVE_MAX_HOURS));
	return static_cast<size_t>(static_cast<double>(bank) * PASSIVE_RATE_PER_HOUR * hoursElapsed);
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	size_t last = 0;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == delimiter) {
			result.emplace_back(str.substr(last, i - last));
			last = i + 1;
		}
	}
	if (last < str.size()) {
		result.emplace_back(str.substr(last, str.size() - last));
	}
	return result;
}

size_t FindUser(const std::vector<User>& users, const std::string& id) {
	for (size_t i = 0; i < users.size(); ++i) {
		if (users[i].id == id) {
			return i;
		}
	}
	return users.size();
}

static std::string ParseMention(const std::string& arg) {
	if (arg.size() < 4 || arg.front() != '<' || arg[1] != '@' || arg.back() != '>') {
		return "";
	}

	size_t start = 2;
	if (arg[start] == '!') {
		++start;
	}
	std::string id = arg.substr(start, arg.size() - start - 1);
	if (id.empty() || !std::all_of(id.begin(), id.end(), ::isdigit)) {
		return "";
	}
	return id;
}

void HandleHelpCommand(const dpp::message_create_t& event) {
	dpp::embed embed;

	embed.set_title("Help")
		.set_description("A list of all available commands")
		.add_field("Jobs",
			"`!job` - View and apply for available jobs\n"
			"`!quit` - Quit your current job\n"
			"`!work` - Work at your current job for money\n"
			"`!promote` - Check your progress towards a promotion or get a promotion"
		)
		.add_field("Economy",
			"`!view` - View your player stats\n"
			"`!deposit <amount>` - Deposit cash into your bank\n"
			"`!withdraw <amount>` - Withdraw cash from your bank\n"			"`!leaderboard` - View the leaderboard of richest players\n"
			"`!passive` - Collect passive income earned from your bank balance\n"
			"`!giveMoney @user <amount>` - Pay a user an amount of money\n"
			"`!rob @user` - Rob another user's cash\n"
		)
		//.add_field("Investment",
			//"`!invest` - Invest your money for passive income\n"
			//"`!business` - Buy and manage businesses\n"
			//"`!collect` - Collect your passive income"
		//)
		.add_field("Gambling",
			"`!coinflip` - Bet on heads or tails for a chance to win money\n"
			"`!roulette` - Gamble away your money at the roulette table\n"
			"`!blackjack` - Gamble away your money at the blackjack table"
			"`!duel @user <amount>` - Create a duel request for another user\n"
			"`!accept` - Accept a pending duel offer"
		)
		.add_field("Other",
			//"`!shop` - Buy and sell items\n"
			"`!daily` - Claim your daily reward\n"
			"`!weekly` - Claim your weekly reward\n"
			//"`!hunt` - Hunt for loot\n"
			//"`!fish` - Go fishing\n"
			//"`!mine` - Mine for resources"
		)
		.add_field("Admin", 
			"`!shutdown` - Shutdown the bot\n"
			"`!adminGiveMoney @user <amount>` - Give a user an amount of money\n"
			"`!adminRemoveMoney @user <amount>` - Remove an amount of money from a user"
		);

	event.reply(dpp::message(event.msg.channel_id, embed));
	return;
}

static std::string FormatCooldown(size_t remaining) {
	size_t hours = remaining / 3600;
	size_t minutes = (remaining % 3600) / 60;
	size_t seconds = remaining % 60;
	if (hours > 0) {
		return std::format("{}h {}m {}s", hours, minutes, seconds);
	}
	return std::format("{}m {}s", minutes, seconds);
};

void HandleViewCommand(User& user, const dpp::message_create_t& event) {
	dpp::embed embed;
	embed.set_title(event.msg.author.username)
		 .add_field("Balance", std::format("Cash: ${}\nBank: ${}", user.cash, user.bank));

	size_t currentTime = static_cast<size_t>(time(nullptr));

	auto TimerStatus = [&](size_t last, size_t cooldown) -> std::string {
		if (last + cooldown <= currentTime) {
			return "Ready";
		}
		return std::format("{}", FormatCooldown((last + cooldown) - currentTime));
	};

	embed.add_field("Timers",
		std::format(
			"Work: {}\nRob: {}\nDaily: {}\nWeekly: {}\n",
			TimerStatus(user.lastWork, SECONDS_IN_HALF_HOUR),
			TimerStatus(user.lastRobbery, SECONDS_IN_HOUR),
			TimerStatus(user.lastDaily, SECONDS_IN_DAY),
			TimerStatus(user.lastWeekly, SECONDS_IN_WEEK)
		)
	);

	size_t pendingPassive = user.passiveBalance + CalcPassiveIncome(user.bank, user.lastPassive);
	embed.add_field("Passive Income",
		std::format("Pending: ${}\n(1% of bank/hour, max {} hours)", pendingPassive, PASSIVE_MAX_HOURS)
	);

	event.reply(dpp::message(event.msg.channel_id, embed));
}

void HandleJobCommand(User& user, const dpp::message_create_t& event) {
	const Job& userJob = JOB_PATHS[user.jobPathIdx][user.jobIdx];
	dpp::embed embed;
	embed.set_title("Job Menu")
	.set_description(std::format("Current Job: {}", (user.hasJob) ? userJob.title : "Unemployed"));

	dpp::component jobDropdown;
	jobDropdown.set_type(dpp::cot_selectmenu)
		.set_id("job_select_menu");

	std::string jobStr = "";

	for (size_t i = 0; i < JOB_PATHS.size(); ++i) {
		jobStr += JOB_PATHS[i][0].title + '\n';
		jobDropdown.add_select_option(dpp::select_option(JOB_PATHS[i][0].title, std::to_string(i), "Starting Position"));
	}

	embed.add_field("Available Jobs", jobStr);

	dpp::message reply(event.msg.channel_id, embed);
	reply.add_component(
		dpp::component().set_type(dpp::cot_action_row).add_component(jobDropdown)
	);

	event.reply(reply);
}

void HandleQuitCommand(User& user, const dpp::message_create_t& event) {
	if (user.hasJob) {
		user.hasJob = false;
		user.jobIdx = 0;
		user.jobPathIdx = 0;
		event.reply(dpp::embed().set_title("Job").set_description("You have quit your job."));
		return;
	}

	event.reply(dpp::embed().set_title("Job").set_description("You have no job to quit."));
}

void HandlePromoteCommand(User& user, const dpp::message_create_t& event) {
	if (user.hasJob) {
		const Job& userJob = JOB_PATHS[user.jobPathIdx][user.jobIdx];

		const size_t jobPathSize = JOB_PATHS[user.jobPathIdx].size();
		if (user.timesWorked >= userJob.promotionThreshold) {
			if (user.jobIdx + 1 >= static_cast<int>(jobPathSize)) {
				event.reply(
					dpp::embed().set_title("Job")
								.set_description("You are already at the highest position in your career path.")
				);
				return;
			}
			user.jobIdx++;
			user.timesWorked = 0;
			const Job& newJob = JOB_PATHS[user.jobPathIdx][user.jobIdx];
			event.reply(
				dpp::embed().set_title("Job")
							.set_description(
								std::format(
									"You have received a promotion.\nYour new title is **{}**.",
									newJob.title
								)
							)
			);
			return;
		}

		int workLeft = userJob.promotionThreshold - user.timesWorked;

		event.reply(
			dpp::embed().set_title("Job")
						.set_description(std::format("You need to work {} more times for a promotion.", workLeft))
		);
	}
	else {
		event.reply(dpp::embed().set_title("Job").set_description("You do not have a job."));
	}
}

void HandleWorkCommand(User& user, const dpp::message_create_t& event) {
	size_t currentTime = static_cast<size_t>(time(nullptr));

	if (user.hasJob) {
		const Job& userJob = JOB_PATHS[user.jobPathIdx][user.jobIdx];
		if (user.lastWork + SECONDS_IN_HALF_HOUR <= currentTime) {
			int hoursWorked = GetRandomInt(userJob.minHours, userJob.maxHours);
			int basePay = hoursWorked * userJob.payPerHour;
			if (DO_DIMINISHING_RETURNS) {
				size_t moneyGained = ApplyDiminishingReturns(user.cash + user.bank, static_cast<size_t>(basePay));
				user.cash += moneyGained;

				dpp::embed embed;
				embed.set_title("Work")
					 .set_description(
						 std::format(
							 "You worked for {} hours and gained ${}.\nYou were taxed ${} based on your bank balance.",
							 hoursWorked, moneyGained,
							 moneyGained - ApplyDiminishingReturns(user.cash + user.bank, basePay)
						 )
					 );

				event.reply(embed);
			}
			else {
				user.cash += basePay;

				dpp::embed embed;
				embed.set_title("Work")
					.set_description(std::format("You worked for {} hours and gained ${}.", hoursWorked, basePay));

				event.reply(embed);
			}

			user.lastWork = currentTime;
			user.timesWorked++;

			return;
		}
		size_t timeRemaining = (user.lastWork + SECONDS_IN_HALF_HOUR) - currentTime;
		size_t minutes = (timeRemaining % 3600) / 60;
		size_t seconds = timeRemaining % 60;

		dpp::embed embed;
		embed.set_title("Work")
			 .set_description(std::format("You have another {}m {}s until you can work again.", minutes, seconds));

		event.reply(embed);
		return;
	}
	event.reply(dpp::embed().set_title("Work").set_description("You have no job to work at."));
}

void HandleLeaderboardCommand(const std::vector<User>& users, const dpp::message_create_t& event) {
	std::vector<User> sorted = users;
	std::sort(sorted.begin(), sorted.end(), [](const User& a, const User& b) {
		return (a.cash + a.bank) > (b.cash + b.bank);
	});

	dpp::embed leaderboard;
	leaderboard.set_title("Leaderboard");

	for (size_t i = 0; i < sorted.size() && i < 10; ++i) {
		dpp::user* user = dpp::find_user(std::stoull(sorted[i].id));
		if (user) {
			std::string username = user->username;
			size_t total = sorted[i].cash + sorted[i].bank;
			leaderboard.add_field(std::format("#{}", i + 1), std::format("{} - ${}", username, total));
		}
	}

	event.reply(dpp::message(event.msg.channel_id, leaderboard));
}

void HandleDailyCommand(User& user, const dpp::message_create_t& event) {
	size_t currentTime = static_cast<size_t>(time(nullptr));
	if (user.lastDaily + SECONDS_IN_DAY <= currentTime) {
		if (user.lastDaily + (SECONDS_IN_DAY * 2) < currentTime) {
			user.dailyStreak = 0;
		}
		else {
			user.dailyStreak++;
		}
		if (DO_DIMINISHING_RETURNS) {
			int gained = std::min(1000, static_cast<int>(std::pow(2, user.dailyStreak)));
			size_t actualGain = ApplyDiminishingReturns(user.cash + user.bank, gained);
			user.cash += actualGain;
			user.lastDaily = currentTime;

			dpp::embed embed;
			embed.set_title("Daily")
				.set_description(std::format("You have received ${} from your daily reward.", actualGain));

			event.reply(dpp::message(event.msg.channel_id, embed));
		}
		else {
			int gained = std::min(1000, static_cast<int>(std::pow(2, user.dailyStreak)));
			user.cash += gained;
			user.lastDaily = currentTime;

			dpp::embed embed;
			embed.set_title("Daily")
				.set_description(std::format("You have received ${} from your daily reward.", gained));

			event.reply(dpp::message(event.msg.channel_id, embed));
		}
	}
	else {
		size_t timeRemaining = (user.lastDaily + SECONDS_IN_DAY) - currentTime;
		size_t hours = timeRemaining / 3600;
		size_t minutes = (timeRemaining % 3600) / 60;
		size_t seconds = timeRemaining % 60;
		dpp::embed embed;
		embed.set_title("Daily")
			.set_description(std::format("Your daily is not ready yet. Please wait {}h {}m {}s.", hours, minutes, seconds));

		event.reply(dpp::message(event.msg.channel_id, embed));
	}
}

void HandleWeeklyCommand(User& user, const dpp::message_create_t& event) {
	size_t currentTime = static_cast<size_t>(time(nullptr));
	if (user.lastWeekly + SECONDS_IN_WEEK <= currentTime) {
		if (user.lastWeekly + (SECONDS_IN_WEEK * 2) < currentTime) {
			user.weeklyStreak = 0;
		}
		else {
			user.weeklyStreak++;
		}
		if (DO_DIMINISHING_RETURNS) {
			int gained = std::min(1000, static_cast<int>(std::pow(2, user.weeklyStreak + 2)));
			size_t actualGain = ApplyDiminishingReturns(user.cash + user.bank, gained);
			user.cash += actualGain;
			user.lastWeekly = currentTime;

			dpp::embed embed;
			embed.set_title("Weekly")
				 .set_description(std::format("You have received ${} from your weekly reward.", actualGain));

			event.reply(dpp::message(event.msg.channel_id, embed));
		}
		else {
			int gained = std::min(1000, static_cast<int>(std::pow(2, user.dailyStreak + 2)));
			user.cash += gained;
			user.lastWeekly = currentTime;

			dpp::embed embed;
			embed.set_title("Weekly")
				 .set_description(std::format("You have received ${} from your weekly reward.", gained));

			event.reply(dpp::message(event.msg.channel_id, embed));
		}
	}
	else {
		size_t timeRemaining = (user.lastWeekly + SECONDS_IN_WEEK) - currentTime;
		size_t hours = timeRemaining / 3600;
		size_t minutes = (timeRemaining % 3600) / 60;
		size_t seconds = timeRemaining % 60;
		dpp::embed embed;
		embed.set_title("Weekly")
			.set_description(std::format("Your weekly is not ready yet. Please wait {}h {}m {}s.", hours, minutes, seconds));

		event.reply(dpp::message(event.msg.channel_id, embed));
	}
}

void HandleCoinflipCommand(User& user, const dpp::message_create_t& event) {
	if (user.activeCoinflip) {
		event.reply(dpp::embed().set_title("Coinflip").set_description("You already have an active coinflip."));
		return;
	}

	user.activeCoinflip = true;
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(
			dpp::embed().set_title("Coinflip")
						.set_description(std::format("!coinflip takes 2 arguments, {} were provided.", arguments.size()))
		);
		user.activeCoinflip = false;
		return;
	}

	int betAmount = 0;

	if (arguments[0] == "all") {
		if (user.cash <= 0) {
			event.reply(
				dpp::embed().set_title("Coinflip")
							.set_description("You have no money.")
			);
			return;
		}
		betAmount = static_cast<int>(user.cash);
	}
	else {
		try {
			betAmount = std::stoi(arguments[0]);
			if (betAmount < 0) {
				throw std::invalid_argument("");
			}
			else if (betAmount > user.cash) {
				event.reply(dpp::embed().set_title("Coinflip").set_description("You do not have enough money for this bet."));
				user.activeCoinflip = false;
				return;
			}
		}
		catch (...) {
			event.reply(dpp::embed().set_title("Coinflip").set_description("Argument 1 (bet amount) was invalid."));
			user.activeCoinflip = false;
			return;
		}
	}

	bool userChoice = false;

	if (arguments[1] == "heads") {
		userChoice = true;
	}
	else if (arguments[1] != "tails") {
		event.reply(dpp::embed().set_title("Coinflip").set_description("Argument 2 (heads/tails) was invalid."));
		user.activeCoinflip = false;
		return;
	}

	bool cfResult = GetRandomInt(0, 1) == 0;

	dpp::embed embed;
	embed.set_title("Coinflip");

	if (cfResult) {
		embed.set_description("The result was heads.");
	}
	else {
		embed.set_description("The result was tails.");
	}

	if (cfResult == userChoice) {
		embed.add_field("Result", "You won the coinflip and doubled your bet!");
		user.cash += betAmount;
	}
	else {
		embed.add_field("Result", "You lost the coinflip.");
		user.cash -= betAmount;
	}

	user.activeCoinflip = false;

	event.reply(embed);
}

static dpp::component MakeHitStandRow() {
	dpp::component hitButton;
	hitButton.set_type(dpp::cot_button)
			 .set_id("hit_button")
			 .set_label("Hit");

	dpp::component standButton;
	standButton.set_type(dpp::cot_button)
			 .set_id("stand_button")
			 .set_label("Stand");

	return dpp::component().set_type(dpp::cot_action_row)
						   .add_component(hitButton)
						   .add_component(standButton);
}

void HandleBlackjackCommand(User& user, const dpp::message_create_t& event) {
	if (user.activeBlackjack) {
		event.reply(dpp::embed().set_title("Blackjack").set_description("You already have an active blackjack game."));
		return;
	}

	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');

	arguments.erase(arguments.begin());

	if (arguments.size() == 0) {
		event.reply(
			dpp::embed().set_title("Blackjack")
						.set_description("Invalid number of arguments entered.\nUsage: !blackjack <bet>")
		);
		return;
	}

	size_t betAmount = 0;

	if (arguments[0] == "all") {
		betAmount = user.cash;
	}
	else {
		try {
			betAmount = std::stoull(arguments[0]);

			if (betAmount == 0) {
				throw std::invalid_argument("");
			}
			if (betAmount > user.cash) {
				dpp::embed embed;
				embed.set_title("Blackjack")
					 .set_description("You do not have enough money for this bet.\nUsage: !blackjack <bet>");

				event.reply(dpp::message(event.msg.channel_id, embed));
				return;
			}
		}
		catch (...) {
			dpp::embed embed;
			embed.set_title("Blackjack")
				 .set_description("Argument 1 (bet) is invalid.\nUsage: !blackjack <bet>");

			event.reply(dpp::message(event.msg.channel_id, embed));
			return;
		}

	}

	user.blackjackGame = BlackjackGame(betAmount);
	user.activeBlackjack = true;

	int playerTotal = HandTotal(user.blackjackGame.playerHand);
	int dealerTotal = HandTotal(user.blackjackGame.dealerHand);

	bool playerBlackjack = (playerTotal == 21);
	bool dealerBlackjack = (dealerTotal == 21);

	if (playerBlackjack || dealerBlackjack) {
		user.activeBlackjack = false;
		dpp::embed embed;
		embed.set_title("Blackjack")
			 .add_field("Your Hand", HandString(user.blackjackGame.playerHand))
			 .add_field("Dealer Hand", HandString(user.blackjackGame.dealerHand));

		if (playerBlackjack && dealerBlackjack) {
			embed.set_description("Both you and the dealer have Blackjack! It's a tie — your bet has been returned.");
		}
		else if (playerBlackjack) {
			size_t payout = betAmount + (betAmount / 2); // 3:2 payout
			user.cash += payout;
			embed.set_description(std::format("**Blackjack!** You win ${}! (3:2 payout)", payout));
		}
		else {
			user.cash -= betAmount;
			embed.set_description(std::format("Dealer has Blackjack! You lost ${}.", betAmount));
		}

		event.reply(dpp::message(event.msg.channel_id, embed));
		return;
	}
	dpp::embed embed;
	embed.set_title("Blackjack")
		 .add_field("Your Total", HandString(user.blackjackGame.playerHand))
		 .add_field("Dealer Total", CardName(user.blackjackGame.dealerHand[0]));

	dpp::message msg(event.msg.channel_id, embed);
	msg.add_component(MakeHitStandRow());

	event.reply(msg);
}

static bool IsNumber(std::string str) {
	try {
		double num = std::stod(str);
	}
	catch (...) {
		return false;
	}
	return true;
}

void HandleRouletteCommand(User& user, const dpp::message_create_t& event) {
	if (user.activeRoulette) {
		event.reply(dpp::embed().set_title("Roulette").set_description("You already have an active roulette."));
		return;
	}

	user.activeRoulette = true;
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(
			dpp::embed().set_title("Roulette")
			.set_description(std::format("!roulette takes 2 arguments, {} were provided.", arguments.size()))
		);
		user.activeRoulette = false;
		return;
	}

	int betAmount = 0;

	if (arguments[0] == "all") {
		if (user.cash <= 0) {
			event.reply(
				dpp::embed().set_title("Roulette")
							.set_description("You have no money.")
			);
			return;
		}
		betAmount = static_cast<int>(user.cash);
	}
	else {
		try {
			betAmount = std::stoi(arguments[0]);
			if (betAmount < 0) {
				throw std::invalid_argument("");
			}
			else if (betAmount > user.cash) {
				event.reply(dpp::embed().set_title("Roulette").set_description("You do not have enough money for this bet."));
				user.activeRoulette = false;
				return;
			}
		}
		catch (...) {
			event.reply(dpp::embed().set_title("Roulette").set_description("Argument 1 (bet amount) was invalid."));
			user.activeRoulette = false;
			return;
		}
	}

	int rouletteNumber = GetRandomInt(0, 36);
	bool won = false;
	bool choiceIsNumber = IsNumber(arguments[1]);

	if (choiceIsNumber) {
		int userChoice = std::stoi(arguments[1]);

		if (userChoice == rouletteNumber) {
			won = true;
		}
	}
	else if (arguments[1].starts_with("red")) {
		const int N = 18;

		const int redNumbers[N] = {
			1, 3, 5, 7, 9, 12, 14, 16, 18, 19,
			21, 23, 25, 27, 30, 32, 34, 36
		};

		bool isRed = false;
		for (int i = 0; i < N; ++i) {
			if (rouletteNumber == redNumbers[i]) {
				isRed = true;
			}
		}

		if (isRed) {
			won = true;
		}
	}
	else if (arguments[1].starts_with("black")) {
		const int N = 18;

		const int blackNumbers[] = {
			2,4,6,8,10,11,13,15,17,20,
			22,24,26,28,29,31,33,35
		};

		bool isBlack = false;
		for (int i = 0; i < N; ++i) {
			if (rouletteNumber == blackNumbers[i]) {
				isBlack = true;
			}
		}

		if (isBlack) {
			won = true;
		}
	}
	else if (arguments[1].starts_with("even")) {
		if (rouletteNumber != 0 && rouletteNumber % 2 == 0) {
			won = true;
		}
	}
	else if (arguments[1].starts_with("odd")) {
		if (rouletteNumber != 0 && rouletteNumber % 2 == 1) {
			won = true;
		}
		
	}
	
	if (won) {
		int payout = 0;
		if (choiceIsNumber) {
			payout = betAmount * 35;
		}
		else {
			payout = betAmount * 2;
		}
		event.reply(
			dpp::embed().set_title("Roulette")
						.set_description(
							std::format(
								"The roulette landed on **{}**. You won ${}!",
								rouletteNumber, payout
							)
						)
		);

		user.cash += payout;
	}
	else {
		if (user.cash >= betAmount) {
			user.cash -= betAmount;
		}
		else {
			user.cash = 0;
		}

		event.reply(
			dpp::embed().set_title("Roulette")
						.set_description(
							std::format(
								"The roulette landed on **{}**. You lost ${}.",
								rouletteNumber, betAmount
							)
						)
		);
	}

	user.activeRoulette = false;
	user.rouletteGame.finished = true;
}

void HandleGiveMoneyCommand(User& issuingUser, std::vector<User>& users, const dpp::message_create_t& event) {
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(dpp::embed().set_title("Give Money").set_description("Invalid number of arguments.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	std::string targetId = ParseMention(arguments[0]);
	if (targetId.empty()) {
		event.reply(dpp::embed().set_title("Give Money").set_description("Argument 1 (@user) is invalid.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	size_t amount = 0;
	try {
		amount = std::stoull(arguments[1]);
		if (amount == 0) {
			throw std::invalid_argument("");
		}
		else if (amount > issuingUser.cash) {
			event.reply(dpp::embed().set_title("Give Money").set_description("You do not have enough money for this."));
			return;
		}
	}
	catch (...) {
		event.reply(dpp::embed().set_title("Give Money").set_description("Argument 2 (amount) is invalid.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	size_t targetIdx = FindUser(users, targetId);
	if (targetIdx == users.size()) {
		users.emplace_back(targetId);
		targetIdx = users.size() - 1;
	}

	issuingUser.cash -= amount;
	users[targetIdx].cash += amount;

	dpp::user* targetDppUser = dpp::find_user(std::stoull(targetId));
	std::string targetName = targetDppUser ? targetDppUser->username : targetId;

	event.reply(
		dpp::embed().set_title("Give Money")
					.set_description(std::format("Gave ${} to {}", amount, targetName))
	);
}

void HandleAdminGiveMoneyCommand(std::vector<User>& users, const dpp::message_create_t& event) {
	if (ADMIN_IDS.find(event.msg.author.id.str()) == ADMIN_IDS.end()) {
		event.reply(dpp::embed().set_title("Admin").set_description("You do not have permission to do this."));
		return;
	}

	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(dpp::embed().set_title("Admin").set_description("Invalid number of arguments.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	std::string targetId = ParseMention(arguments[0]);
	if (targetId.empty()) {
		event.reply(dpp::embed().set_title("Admin").set_description("Argument 1 (@user) is invalid.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	size_t amount = 0;
	try {
		amount = std::stoull(arguments[1]);
		if (amount == 0) {
			throw std::invalid_argument("");
		}
	}
	catch (...) {
		event.reply(dpp::embed().set_title("Admin").set_description("Argument 2 (amount) is invalid.\nUsage: !giveMoney @user <amount>"));
		return;
	}

	size_t targetIdx = FindUser(users, targetId);
	if (targetIdx == users.size()) {
		users.emplace_back(targetId);
		targetIdx = users.size() - 1;
	}

	users[targetIdx].cash += amount;

	dpp::user* targetDppUser = dpp::find_user(std::stoull(targetId));
	std::string targetName = targetDppUser ? targetDppUser->username : targetId;

	event.reply(
		dpp::embed().set_title("Admin")
					.set_description(std::format("Gave ${} to {}", amount, targetName))
	);
}

void HandleAdminRemoveMoneyCommand(std::vector<User>& users, const dpp::message_create_t& event) {
	if (ADMIN_IDS.find(event.msg.author.id.str()) == ADMIN_IDS.end()) {
		event.reply(dpp::embed().set_title("Admin").set_description("You do not have permission to do this."));
		return;
	}

	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(dpp::embed().set_title("Admin").set_description("Invalid number of arguments.\nUsage: !removeMoney @user <amount>"));
		return;
	}

	std::string targetId = ParseMention(arguments[0]);
	if (targetId.empty()) {
		event.reply(dpp::embed().set_title("Admin").set_description("Argument 1 (@user) is invalid.\nUsage: !removeMoney @user <amount>"));
		return;
	}

	size_t amount = 0;
	try {
		amount = std::stoull(arguments[1]);
		if (amount == 0) {
			throw std::invalid_argument("");
		}
	}
	catch (...) {
		event.reply(dpp::embed().set_title("Admin").set_description("Argument 2 (amount) is invalid.\nUsage: !removeMoney @user <amount>"));
		return;
	}

	size_t targetIdx = FindUser(users, targetId);
	if (targetIdx == users.size()) {
		event.reply(dpp::embed().set_title("Admin").set_description("That user has no account."));
		return;
	}

	User& target = users[targetIdx];

	if (amount > target.cash) {
		amount = target.cash;
	}
	target.cash -= amount;

	dpp::user* targetDppUser = dpp::find_user(std::stoull(targetId));
	std::string targetName = targetDppUser ? targetDppUser->username : targetId;

	event.reply(
		dpp::embed().set_title("Admin")
					.set_description(std::format("Removed ${} from {}", amount, targetName))
	);
}

void HandleRobCommand(User& issuingUser, std::vector<User>& users, const dpp::message_create_t& event) {
	size_t currentTime = static_cast<size_t>(time(nullptr));

	if (issuingUser.lastRobbery + SECONDS_IN_HALF_HOUR > currentTime) {
		size_t timeRemaining = (issuingUser.lastRobbery + SECONDS_IN_HOUR) - currentTime;
		size_t minutes = (timeRemaining % 3600) / 60;
		size_t seconds = timeRemaining % 60;

		dpp::embed embed;
		embed.set_title("Robbery")
			.set_description(std::format("You have another {}m {}s until you can rob again.", minutes, seconds));

		event.reply(embed);
		return;
	}

	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 1) {
		event.reply(dpp::embed().set_title("Robbery").set_description("Invalid number of arguments.\nUsage: !rob @user"));
		return;
	}

	std::string targetId = ParseMention(arguments[0]);
	if (targetId.empty()) {
		event.reply(dpp::embed().set_title("Robbery").set_description("Argument 1 (@user) is invalid.\nUsage: !rob @user"));
		return;
	}

	if (targetId == issuingUser.id) {
		event.reply(dpp::embed().set_title("Robbery").set_description("You cannot rob yourself."));
		return;
	}

	size_t targetIdx = FindUser(users, targetId);
	if (targetIdx == users.size()) {
		users.emplace_back(targetId);
		targetIdx = users.size() - 1;
	}

	if (users[targetIdx].cash == 0) {
		event.reply(dpp::embed().set_title("Robbery").set_description("That user has no cash to steal."));
		return;
	}

	constexpr int STEAL_CHANCE = 3;
	bool stealSuccessful = GetRandomInt(1, 10) <= STEAL_CHANCE;

	issuingUser.lastRobbery = currentTime;

	if (stealSuccessful) {
		if (users[targetIdx].cash <= 0) {
			event.reply(
				dpp::embed().set_title("Robbery")
							.set_description("The target has no money.")
			);
			return;
		}

		constexpr int STEAL_AMOUNT_PERCENT_LOWER = 1, STEAL_AMOUNT_PERCENT_UPPER = 3;
		float percentStolen = GetRandomInt(STEAL_AMOUNT_PERCENT_LOWER, STEAL_AMOUNT_PERCENT_UPPER) / 10.0f;

		size_t cashStolen = static_cast<size_t>(users[targetIdx].cash * percentStolen);
		cashStolen = std::max(static_cast<size_t>(1), cashStolen);

		if (cashStolen <= users[targetIdx].cash) {
			users[targetIdx].cash -= cashStolen;
		}
		else {
			users[targetIdx].cash = 0;
		}

		issuingUser.cash += cashStolen;

		event.reply(
			dpp::embed().set_title("Robbery")
						.set_description(std::format("You stole ${}!", cashStolen))
		);
		return;
	}
	else {
		event.reply(
			dpp::embed().set_title("Robbery")
						.set_description("Your robbery attempt failed. Better luck next time.")
		);
	}
}

void HandleDepositCommand(User& user, const dpp::message_create_t& event) {
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 1) {
		event.reply(dpp::embed().set_title("Deposit").set_description("Invalid number of arguments.\nUsage: !deposit <amount>"));
		return;
	}

	size_t amount = 0;

	if (arguments[0] == "all") {
		if (user.cash == 0) {
			event.reply(dpp::embed().set_title("Deposit").set_description("You have no cash to deposit."));
			return;
		}
		amount = user.cash;
	}
	else {
		try {
			amount = std::stoull(arguments[0]);

			if (amount == 0) {
				throw std::invalid_argument("");
			}
		}
		catch (...) {
			event.reply(dpp::embed().set_title("Deposit").set_description("Argument 1 (amount) is invalid.\nUsage: !deposit <amount>"));
			return;
		}

		if (amount > user.cash) {
			event.reply(
				dpp::embed().set_title("Deposit")
				.set_description(std::format("You do not have enough money to deposit ${}.", amount))
			);
			return;
		}
	}

	user.cash -= amount;
	user.bank += amount;

	event.reply(
		dpp::embed().set_title("Deposit")
					.set_description(std::format("You have deposited ${}.", amount))
	);
}

void HandleWithdrawCommand(User& user, const dpp::message_create_t& event) {
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 1) {
		event.reply(dpp::embed().set_title("Withdraw").set_description("Invalid number of arguments.\nUsage: !deposit <amount>"));
		return;
	}

	size_t amount = 0;

	if (arguments[0] == "all") {
		if (user.bank == 0) {
			event.reply(dpp::embed().set_title("Withdraw").set_description("You have no cash to withdraw."));
			return;
		}
		amount = user.bank;
	}
	else {
		try {
			amount = std::stoull(arguments[0]);

			if (amount == 0) {
				throw std::invalid_argument("");
			}
		}
		catch (...) {
			event.reply(dpp::embed().set_title("Withdraw").set_description("Argument 1 (amount) is invalid.\nUsage: !deposit <amount>"));
			return;
		}

		if (amount > user.bank) {
			event.reply(
				dpp::embed().set_title("Withdraw")
							.set_description(std::format("You do not have enough money to withdraw ${}.", amount))
			);
			return;
		}
	}

	user.bank -= amount;
	user.cash += amount;

	event.reply(
		dpp::embed().set_title("Withdraw")
					.set_description(std::format("You have withdrawn ${}.", amount))
	);
}

void HandleDuelCommand(std::unordered_map<std::string, PendingDuel>& pendingDuels, User& challenger, std::vector<User>& users, const dpp::message_create_t& event) {
	std::vector<std::string> arguments = SplitString(event.msg.content, ' ');
	arguments.erase(arguments.begin());

	if (arguments.size() < 2) {
		event.reply(dpp::embed().set_title("Duel").set_description("Invalid number of arguments.\nUsage: !duel @user <amount>"));
		return;
	}

	std::string targetId = ParseMention(arguments[0]);
	if (targetId.empty()) {
		event.reply(dpp::embed().set_title("Duel").set_description("Argument 1 (@user) is invalid.\nUsage: !duel @user <amount>"));
		return;
	}

	if (targetId == challenger.id) {
		event.reply(dpp::embed().set_title("Duel").set_description("You cannot duel yourself."));
		return;
	}

	size_t bet = 0;
	try {
		bet = std::stoull(arguments[1]);
		if (bet == 0) {
			throw std::invalid_argument("");
		}
	}
	catch (...) {
		event.reply(dpp::embed().set_title("Duel").set_description("Argument 2 (amount) is invalid.\nUsage: !duel @user <amount>"));
		return;
	}

	if (bet > challenger.cash) {
		event.reply(dpp::embed().set_title("Duel").set_description("You do not have enough cash to place that bet."));
		return;
	}

	auto existing = pendingDuels.find(targetId);
	if (existing != pendingDuels.end()) {
		size_t currentTime = static_cast<size_t>(time(nullptr));
		if (existing->second.expiresAt > currentTime) {
			event.reply(dpp::embed().set_title("Duel").set_description("That user already has a pending duel challenge."));
			return;
		}
		pendingDuels.erase(existing);
	}

	size_t targetIdx = FindUser(users, targetId);
	if (targetIdx == users.size()) {
		users.emplace_back(targetId);
		targetIdx = users.size() - 1;
	}

	if (users[targetIdx].cash < bet) {
		event.reply(dpp::embed().set_title("Duel").set_description("That user does not have enough cash to match the bet."));
		return;
	}

	pendingDuels[targetId] = PendingDuel(challenger.id, bet);

	dpp::user* targetDppUser = dpp::find_user(std::stoull(targetId));
	std::string targetName = targetDppUser ? targetDppUser->username : targetId;

	event.reply(
		dpp::embed().set_title("Duel")
		.set_description(
			std::format(
				"<@{}> has challenged **{}** to a duel for ${}!\n"
				"**{}**, type `!accept` within {} seconds to accept.",
				challenger.id, targetName, bet,
				targetName, DUEL_EXPIRY_SECONDS
			)
		)
	);
}

void HandleAcceptCommand(std::unordered_map<std::string, PendingDuel>& pendingDuels, User& acceptingUser, std::vector<User>& users, const dpp::message_create_t& event) {
	auto it = pendingDuels.find(acceptingUser.id);
	if (it == pendingDuels.end()) {
		event.reply(dpp::embed().set_title("Duel").set_description("You have no pending duel challenge."));
		return;
	}

	size_t currentTime = static_cast<size_t>(time(nullptr));
	if (it->second.expiresAt <= currentTime) {
		pendingDuels.erase(it);
		event.reply(dpp::embed().set_title("Duel").set_description("Your duel challenge has expired."));
		return;
	}

	const std::string challengerId = it->second.challengerId;
	const size_t bet = it->second.bet;
	pendingDuels.erase(it);

	size_t challengerIdx = FindUser(users, challengerId);
	if (challengerIdx == users.size()) {
		event.reply(dpp::embed().set_title("Duel").set_description("The challenger no longer has an account."));
		return;
	}

	User& challenger = users[challengerIdx];

	if (challenger.cash < bet) {
		event.reply(dpp::embed().set_title("Duel").set_description("The challenger no longer has enough cash to cover the bet."));
		return;
	}
	if (acceptingUser.cash < bet) {
		event.reply(dpp::embed().set_title("Duel").set_description("You no longer have enough cash to cover the bet."));
		return;
	}

	int challengerRoll = GetRandomInt(1, 100);
	int acceptorRoll = GetRandomInt(1, 100);

	dpp::user* challengerDppUser = dpp::find_user(std::stoull(challengerId));
	std::string challengerName = challengerDppUser ? challengerDppUser->username : challengerId;
	std::string acceptorName = event.msg.author.username;

	dpp::embed embed;
	embed.set_title("Duel")
		 .add_field(std::format("{} rolled", challengerName), std::to_string(challengerRoll), true)
		 .add_field(std::format("{} rolled", acceptorName), std::to_string(acceptorRoll), true);

	if (challengerRoll > acceptorRoll) {
		challenger.cash += bet;
		acceptingUser.cash -= bet;
		embed.set_description(std::format("**{}** wins the duel and takes ${}!", challengerName, bet));
	}
	else if (acceptorRoll > challengerRoll) {
		acceptingUser.cash += bet;
		challenger.cash -= bet;
		embed.set_description(std::format("**{}** wins the duel and takes ${}!", acceptorName, bet));
	}
	else {
		embed.set_description("It's a tie — no money changes hands.");
	}

	event.reply(dpp::message(event.msg.channel_id, embed));
}

void HandlePassiveCommand(User& user, const dpp::message_create_t& event) {
	size_t currentTime = static_cast<size_t>(time(nullptr));

	size_t accrued = CalcPassiveIncome(user.bank, user.lastPassive);
	user.passiveBalance += accrued;
	user.lastPassive = currentTime;

	if (user.passiveBalance == 0) {
		dpp::embed embed;
		embed.set_title("Passive Income")
			.set_description(
				user.bank == 0
				? "You have no money in the bank to earn passive income from."
				: "You have no passive income to collect yet. Check back soon."
			);
		event.reply(dpp::message(event.msg.channel_id, embed));
		return;
	}

	size_t collected = user.passiveBalance;
	user.cash += collected;
	user.passiveBalance = 0;

	dpp::embed embed;
	embed.set_title("Passive Income")
		 .set_description(std::format(
		 	 "You collected **${}** in passive income from your bank balance.\n"
		 	 "*({}% of your bank per hour, up to {} hours)*",
		 	 collected,
		 	 static_cast<int>(PASSIVE_RATE_PER_HOUR * 100),
		 	 PASSIVE_MAX_HOURS
		 ));

	event.reply(dpp::message(event.msg.channel_id, embed));
}

int main() {
	char* tokenEnv = nullptr;
	size_t len = 0;
	_dupenv_s(&tokenEnv, &len, "TOKEN");

	const std::string BOT_TOKEN = tokenEnv ? tokenEnv : "";
	if (tokenEnv) {
		free(tokenEnv);
	}
	if (BOT_TOKEN.empty()) {
		throw std::runtime_error("Could not find TOKEN env variable");
	}

	dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);

	std::vector<User> users;

	std::unordered_map<std::string, PendingDuel> pendingDuels;

	LoadUsers(users);

	std::atomic<bool> running(true);

	std::thread saveThread([&users, &running]() {
		while (running) {
			for (int i = 0; i < 120; ++i) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				if (!running) {
					break;
				}
			}
			if (running) {
				SaveUsers(users);
			}
		}
		});

	bot.on_log(dpp::utility::cout_logger());

	bot.on_message_create([&users, &bot, &running, &saveThread, &pendingDuels](const dpp::message_create_t& event) {
		std::string message = event.msg.content;

		if (!message.starts_with('!')) {
			return;
		}

		size_t foundIdx = FindUser(users, event.msg.author.id.str());
		if (foundIdx == users.size()) {
			users.emplace_back(event.msg.author.id.str());
			foundIdx = users.size() - 1;
		}
		User& user = users[foundIdx];

		if (message.starts_with("!help")) {
			HandleHelpCommand(event);
			return;
		}

		if (message.starts_with("!view") || message.starts_with("!balance") || message.starts_with("!bal")) {
			HandleViewCommand(user, event);
			return;
		}

		if (message.starts_with("!job") || message.starts_with("!jobs")) {
			HandleJobCommand(user, event);
			return;
		};

		if (message.starts_with("!quit")) {
			HandleQuitCommand(user, event);
			return;
		}

		if (message.starts_with("!promote")) {
			HandlePromoteCommand(user, event);
			return;
		}

		if (message.starts_with("!work")) {
			HandleWorkCommand(user, event);
			return;
		}

		if (message.starts_with("!leaderboard") || message.starts_with("!lb")) {
			HandleLeaderboardCommand(users, event);
			return;
		}

		if (message.starts_with("!daily")) {
			HandleDailyCommand(user, event);
			return;
		}

		if (message.starts_with("!weekly")) {
			HandleWeeklyCommand(user, event);
			return;
		}

		if (message.starts_with("!coinflip") || message.starts_with("!cf")) {
			HandleCoinflipCommand(user, event);
			return;
		}

		if (message.starts_with("!blackjack") || message.starts_with("!bj")) {
			HandleBlackjackCommand(user, event);
			return;
		}

		if (message.starts_with("!roulette")) {
			HandleRouletteCommand(user, event);
			return;
		}

		if (message.starts_with("!giveMoney")) {
			HandleGiveMoneyCommand(user, users, event);
			return;
		}

		if (message.starts_with("!rob")) {
			HandleRobCommand(user, users, event);
			return;
		}

		if (message.starts_with("!deposit")) {
			HandleDepositCommand(user, event);
			return;
		}

		if (message.starts_with("!withdraw")) {
			HandleWithdrawCommand(user, event);
			return;
		}

		if (message.starts_with("!duel")) {
			HandleDuelCommand(pendingDuels, user, users, event);
			return;
		}

		if (message.starts_with("!accept")) {
			HandleAcceptCommand(pendingDuels, user, users, event);
			return;
		}

		if (message.starts_with("!passive")) {
			HandlePassiveCommand(user, event);
			return;
		}

		if (message.starts_with("!adminGiveMoney")) {
			HandleAdminGiveMoneyCommand(users, event);
			return;
		}

		if (message.starts_with("!adminRemoveMoney")) {
			HandleAdminRemoveMoneyCommand(users, event);
			return;
		}

		if (message.starts_with("!shutdown")) {
			if (ADMIN_IDS.find(event.msg.author.id.str()) != ADMIN_IDS.end()) {
				running.store(false);
				event.reply(dpp::embed().set_title("Shutting Down..."));
				SaveUsers(users);
				saveThread.join();

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				bot.shutdown();
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				exit(0);
			}
			else {
				event.reply(dpp::embed().set_title("Good try buster."));
			}
		}
	});

	bot.on_button_click([&users](const dpp::button_click_t& event) {
		std::string userId = event.command.get_issuing_user().id.str();
		size_t userIdx = FindUser(users, userId);
		if (userIdx == users.size()) {
			return;
		}
		User& user = users[userIdx];

		if (event.custom_id == "hit_button") {
			if (!user.activeBlackjack) {
				event.reply(dpp::embed().set_title("Blackjack").set_description("You have no active blackjack game."));
				return;
			}

			user.blackjackGame.playerHand.push_back(user.blackjackGame.draw_card());
			int playerTotal = HandTotal(user.blackjackGame.playerHand);

			if (playerTotal > 21) {
				user.cash -= user.blackjackGame.bet;
				user.activeBlackjack = false;

				event.reply(
					dpp::ir_update_message,
					dpp::embed().set_title("Blackjack")
					.set_description(
						std::format(
							"You busted with **{}**. You lost ${}.",
							HandString(user.blackjackGame.playerHand),
							user.blackjackGame.bet
						)
					)
				);
				return;
			}

			dpp::embed embed;
			embed.set_title("Blackjack")
				.add_field("Your Hand", HandString(user.blackjackGame.playerHand))
				.add_field("Dealer Shows", CardName(user.blackjackGame.dealerHand[0]));

			dpp::message msg(event.command.channel_id, embed);
			msg.add_component(MakeHitStandRow());

			event.reply(dpp::ir_update_message, msg);
			return;
		}

		if (event.custom_id == "stand_button") {
			if (!user.activeBlackjack) {
				event.reply(dpp::embed().set_title("Blackjack").set_description("You have no active blackjack game."));
				return;
			}

			while (HandTotal(user.blackjackGame.dealerHand) < 17) {
				user.blackjackGame.dealerHand.push_back(user.blackjackGame.draw_card());
			}

			int playerTotal = HandTotal(user.blackjackGame.playerHand);
			int dealerTotal = HandTotal(user.blackjackGame.dealerHand);

			dpp::embed embed;
			embed.set_title("Blackjack")
				.add_field("Your Hand", HandString(user.blackjackGame.playerHand))
				.add_field("Dealer Hand", HandString(user.blackjackGame.dealerHand));

			if (dealerTotal > 21) {
				user.cash += user.blackjackGame.bet;
				embed.set_description(std::format("Dealer busted with **{}**.\nYou won ${}!", dealerTotal, user.blackjackGame.bet));
			}
			else if (dealerTotal > playerTotal) {
				embed.set_description(std::format("The dealer won with a hand of {}.\nYou lost ${}.", dealerTotal, user.blackjackGame.bet));
				user.cash -= user.blackjackGame.bet;
			}
			else if (dealerTotal < playerTotal) {
				embed.set_description(std::format("You win with **{}** vs dealer's **{}**!\nYou gained ${}!", playerTotal, dealerTotal, user.blackjackGame.bet));
				user.cash += user.blackjackGame.bet;
			}
			else {
				embed.set_description(std::format("Push - both **{}**. Your bet has been returned.", playerTotal));
			}

			user.activeBlackjack = false;
			event.reply(dpp::ir_update_message, dpp::message(event.command.channel_id, embed));
			return;
		}
	});

	bot.on_select_click([&users](const dpp::select_click_t& event) {
		size_t userIdx = FindUser(users, event.command.get_issuing_user().id.str());
		if (userIdx == users.size()) {
			users.emplace_back(event.command.get_issuing_user().id.str());
		}
		auto& user = users[userIdx];

		if (event.custom_id == "job_select_menu") {
			int selectedPathIdx = std::stoi(event.values[0]);

			user.jobPathIdx = selectedPathIdx;
			user.jobIdx = 0;
			user.hasJob = true;

			const Job& userJob = JOB_PATHS[user.jobPathIdx][user.jobIdx];
			dpp::message response(event.command.channel_id, std::format("Congratulations! You are now working as a **{}**.", userJob.title));
			event.reply(dpp::ir_update_message, response);
		}
	});

	bot.start();
}
