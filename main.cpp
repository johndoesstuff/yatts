// having one monolithic file is a lost art

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

std::string get_current_date() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm = *std::localtime(&in_time_t);
	std::stringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d");
	return ss.str();
}

class Task {
	public:
		std::string name;
		int points;
		bool is_limited;
		int daily_limit;

		Task(std::string n, int p, bool lim = false, int lim_count = 0)
			: name(n), points(p), is_limited(lim), daily_limit(lim_count) {}
};

class CompletionTracker {
	private:
		std::map<std::string, std::map<std::string, int>> daily_completions; // date -> task_name -> count
		std::map<std::string, std::vector<std::pair<std::string, int>>> history; // date -> vector<pair<task_name, points>>
		int total_points = 0;
		const std::string history_file = "history.txt";

	public:
		CompletionTracker() {
			load_history();
		}

		bool can_complete(const Task& task, const std::string& date) {
			if (!task.is_limited) return true;
			auto& day = daily_completions[date];
			return day[task.name] < task.daily_limit;
		}

		void complete(const Task& task, const std::string& date) {
			if (!can_complete(task, date)) {
				std::cout << "Cannot complete " << task.name << ", daily limit reached." << std::endl;
				return;
			}
			auto& day_comp = daily_completions[date];
			day_comp[task.name]++;
			auto& hist = history[date];
			hist.push_back({task.name, task.points});
			total_points += task.points;
			save_history(task, date);
			std::cout << "Completed " << task.name << ", earned " << task.points << " points." << std::endl;
		}

		void view_history(const std::string& date) {
			auto it = history.find(date);
			if (it == history.end()) {
				std::cout << "No history for " << date << "." << std::endl;
				return;
			}
			int day_points = 0;
			std::cout << "History for " << date << ":" << std::endl;
			for (const auto& p : it->second) {
				std::cout << "  " << p.first << ": " << p.second << " points" << std::endl;
				day_points += p.second;
			}
			std::cout << "Total points for the day: " << day_points << std::endl;
		}

		int get_total_points() const {
			return total_points;
		}

	private:
		void load_history() {
			std::ifstream file(history_file);
			if (!file.is_open()) return;

			std::string line;
			while (std::getline(file, line)) {
				std::istringstream iss(line);
				std::string date, task_name;
				int points;
				if (std::getline(iss, date, ',') && std::getline(iss, task_name, ',') && iss >> points) {
					history[date].push_back({task_name, points});
					daily_completions[date][task_name]++;
					total_points += points;
				}
			}
			file.close();
		}

		void save_history(const Task& task, const std::string& date) {
			std::ofstream file(history_file, std::ios::app);
			if (file.is_open()) {
				file << date << "," << task.name << "," << task.points << "\n";
				file.close();
			} else {
				std::cout << "Error: Could not save to history file." << std::endl;
			}
		}
};

class TaskManager {
	private:
		std::vector<Task> tasks;
		const std::string task_file = "tasks.txt";

	public:
		TaskManager() {
			load_tasks();
		}

		void add_task(const Task& task) {
			tasks.push_back(task);
			save_tasks();
		}

		const std::vector<Task>& get_tasks() const {
			return tasks;
		}

	private:
		void load_tasks() {
			std::ifstream file(task_file);
			if (!file.is_open()) return;

			std::string line;
			while (std::getline(file, line)) {
				std::istringstream iss(line);
				std::string name;
				std::string junk;
				int points, is_limited, daily_limit;

				std::getline(iss, name, ',') && iss >> points;
				std::getline(iss, junk, ',') && iss >> is_limited;
				std::getline(iss, junk, ',') && iss >> daily_limit;
				bool limited = is_limited == 1;
				tasks.emplace_back(name, points, limited, limited ? daily_limit : 0);
			}
			file.close();
		}

		void save_tasks() {
			std::ofstream file(task_file);
			if (file.is_open()) {
				for (const auto& t : tasks) {
					file << t.name << "," << t.points << "," << (t.is_limited ? 1 : 0) << "," << t.daily_limit << "\n";
				}
				file.close();
			} else {
				std::cout << "Error: Could not save to tasks file." << std::endl;
			}
		}
};

int main() {
	TaskManager task_manager;
	CompletionTracker tracker;

	std::cout << "Yet Another Task Tracking System" << std::endl;
	std::cout << "Commands:" << std::endl;
	std::cout << "  add <name> <points> <limited 0/1> [daily_limit if limited]" << std::endl;
	std::cout << "  complete <name>" << std::endl;
	std::cout << "  history <date or today>" << std::endl;
	std::cout << "  total" << std::endl;
	std::cout << "  list" << std::endl;
	std::cout << "  quit" << std::endl;

	std::string line;
	while (std::cout << "==========" << std::endl && std::getline(std::cin, line)) {
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;
		std::cout << "==========" << std::endl;

		if (cmd == "add" || cmd == "a") {
			std::string name;
			int points;
			int lim_int;
			int daily_lim = 0;
			if (!(iss >> name)) {
				std::cout << "Expected name" << std::endl;
				continue;
			}
			if (!(iss >> points)) {
				std::cout << "Expected points" << std::endl;
				continue;
			}
			if (!(iss >> lim_int)) {
				lim_int = 0;
			}
			bool limited = (lim_int == 1);
			if (limited && iss >> daily_lim) {
				// daily_lim read
			}
			task_manager.add_task(Task(name, points, limited, daily_lim));
			std::cout << "Added task: " << name << std::endl;
		} else if (cmd == "complete" || cmd == "c") {
			std::string name;
			int times_completed = 1;
			if (!(iss >> name)) {
				std::cout << "Expected name" << std::endl;
				continue;
			}
			if (!(iss >> times_completed)) times_completed = 1;
			auto date = get_current_date();
			auto tasks = task_manager.get_tasks();
			auto it = std::find_if(tasks.begin(), tasks.end(), [&name](const Task& t) { return t.name == name; });
			if (it != tasks.end()) {
				for (int i = 0; i < times_completed; i++) tracker.complete(*it, date);
			} else {
				std::cout << "Task not found." << std::endl;
			}
		} else if (cmd == "history" || cmd =="h" || cmd == "today" || cmd == "yesterday") {
			std::string date_str;
			if (!(iss >> date_str) && cmd != "today" && cmd != "yesterday") {
				std::cout << "Expected date" << std::endl;
				continue;
			}
			if (date_str == "today" || cmd == "today") date_str = get_current_date();
			else if (date_str == "yesterday" || cmd == "yesterday") {
				auto now = std::chrono::system_clock::now();
				auto yesterday = now - std::chrono::hours(24);
				auto in_time_t = std::chrono::system_clock::to_time_t(yesterday);
				std::tm tm = *std::localtime(&in_time_t);
				std::stringstream ss;
				ss << std::put_time(&tm, "%Y-%m-%d");
				date_str = ss.str();
			}
			tracker.view_history(date_str);
		} else if (cmd == "total") {
			std::cout << "Total points: " << tracker.get_total_points() << std::endl;
		} else if (cmd == "list" || cmd == "ls") {
			std::cout << "Tasks:" << std::endl;
			for (const auto& t : task_manager.get_tasks()) {
				std::cout << "  " << t.name << " (" << t.points << " pts)";
				if (t.is_limited) std::cout << " [limited: " << t.daily_limit << "/day]";
				std::cout << std::endl;
			}
		} else if (cmd == "quit" || cmd == "exit" || cmd == "q" || cmd == "e") {
			break;
		} else {
			std::cout << "Unknown command." << std::endl;
		}
	}

	return 0;
}
