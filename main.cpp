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
#include <readline/readline.h>
#include <readline/history.h>

std::string get_current_date() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm = *std::localtime(&in_time_t);
	std::stringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d");
	return ss.str();
}

std::string indent = "    ";

class Task {
	public:
		std::string name;
		double points;
		bool is_limited;
		int daily_limit;
		bool timer_based;

		Task(std::string n, double p, bool lim = false, int lim_count = 0, bool tb = false)
			: name(n), points(p), is_limited(lim), daily_limit(lim_count), timer_based(tb) {}
};

class CompletionTracker {
	private:
		std::map<std::string, std::map<std::string, int>> daily_completions; // date -> task_name -> count
		std::map<std::string, std::vector<std::pair<std::string, double>>> history; // date -> vector<pair<task_name, points>>
		double total_points = 0;
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

		int completions_day(const Task& task, const std::string& date) {
			auto& day = daily_completions[date];
			return day[task.name];
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
			double day_points = 0;
			std::cout << "History for " << date << ":" << std::endl;

			const auto& entries = it->second;

			std::string last_task = entries[0].first;
			double last_points = entries[0].second;
			int count = 1;

			auto flush_task = [&](const std::string& task, double pts, int cnt) {
				if (cnt > 1) std::cout << indent << cnt << "x " << task << ": " << cnt*pts << " points (" << pts << " per)" << std::endl;
				else std::cout << indent << task << ": " << pts << " points" << std::endl;
				day_points += pts * cnt;
			};

			for (int i = 1; i < entries.size(); i++) {
				const auto& [task_name, points] = entries[i];
				if (task_name == last_task && points == last_points) {
					count++;
				} else {
					flush_task(last_task, last_points, count);
					last_task = task_name;
					last_points = points;
					count = 1;
				}
			}
			flush_task(last_task, last_points, count);

			std::cout << "Total points for the day: " << day_points << std::endl;
		}

		double get_total_points() const {
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
				double points;
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
				double points;
				int is_limited, daily_limit, timer_based;

				std::getline(iss, name, ',') && iss >> points;
				std::getline(iss, junk, ',') && iss >> is_limited;
				std::getline(iss, junk, ',') && iss >> daily_limit;
				std::getline(iss, junk, ',') && iss >> timer_based;
				bool limited = is_limited == 1;
				tasks.emplace_back(name, points, limited, limited ? daily_limit : 0, timer_based);
			}
			file.close();
		}

		void save_tasks() {
			std::ofstream file(task_file);
			if (file.is_open()) {
				for (const auto& t : tasks) {
					file << t.name << "," << t.points << "," << (t.is_limited ? 1 : 0) << "," << t.daily_limit << "," << (t.timer_based ? 1 : 0) << "\n";
				}
				file.close();
			} else {
				std::cout << "Error: Could not save to tasks file." << std::endl;
			}
		}
};

TaskManager task_manager;
CompletionTracker tracker;

char* generator_from_vector(const char* text, int state, const std::vector<std::string>& list) {
	static size_t index;
	static size_t len;

	if (state == 0) { // reset search
		index = 0;
		len = strlen(text);
	}

	while (index < list.size()) {
		const std::string& candidate = list[index++];
		if (candidate.compare(0, len, text) == 0)
			return strdup(candidate.c_str()); // must return malloc'ed string
	}
	return nullptr;
}

std::vector<std::string> commands = {
	"start", "complete", "add", "history", "exit", "quit", "today", "yesterday", "list"
};

std::vector<std::string> filters = {
	"nolimit", "notimer", "timer", "limited"
};

char* command_generator(const char* text, int state) {
	return generator_from_vector(text, state, commands);
}

char* filter_generator(const char* text, int state) {
	return generator_from_vector(text, state, filters);
}

char* static_task_generator(const char* text, int state) {
	auto tasks = task_manager.get_tasks();
	std::vector<Task> filtered;
	std::copy_if(tasks.begin(), tasks.end(), std::back_inserter(filtered),
			[](const Task& t) { return !t.timer_based; });
	std::vector<std::string> task_names;
	task_names.reserve(filtered.size());
	std::transform(filtered.begin(), filtered.end(), std::back_inserter(task_names),
			[](const Task& t) { return t.name; });
	return generator_from_vector(text, state, task_names);
}

char* timer_task_generator(const char* text, int state) {
	auto tasks = task_manager.get_tasks();
	std::vector<Task> filtered;
	std::copy_if(tasks.begin(), tasks.end(), std::back_inserter(filtered),
			[](const Task& t) { return t.timer_based; });
	std::vector<std::string> task_names;
	task_names.reserve(filtered.size());
	std::transform(filtered.begin(), filtered.end(), std::back_inserter(task_names),
			[](const Task& t) { return t.name; });
	return generator_from_vector(text, state, task_names);
}

char** completion_callback(const char* text, int start, int end) {
	if (start == 0) return rl_completion_matches(text, command_generator);

	std::string line(rl_line_buffer);
	std::string first_word = line.substr(0, line.find(' '));

	if (first_word == "complete" || first_word == "c") return rl_completion_matches(text, static_task_generator);
	if (first_word == "start" || first_word == "s") return rl_completion_matches(text, timer_task_generator);
	if (first_word == "list" || first_word == "ls" || first_word == "l") return rl_completion_matches(text, filter_generator);

	rl_attempted_completion_over = 1;

	return nullptr;
}

int main() {
	rl_attempted_completion_function = completion_callback;

	std::cout << "Yet Another Task Tracking System" << std::endl;
	std::cout << "Commands:" << std::endl;
	std::cout << indent << "add <name> <points> [limited 0/1] [daily_limit] [timer based 0/1]" << std::endl;
	std::cout << indent << "complete <name> [times]" << std::endl;
	std::cout << indent << "start <name>" << std::endl;
	std::cout << indent << "history <date or today>" << std::endl;
	std::cout << indent << "list" << std::endl;
	std::cout << indent << "quit" << std::endl;

	std::string line;
	while (true) {
		char* input = readline("====================\n");
		if (!input) break;

		std::string line(input);
		free(input);

		if (!line.empty()) add_history(line.c_str());
		std::istringstream iss(line);
		std::string cmd;

		// shortcuts
		auto pos = iss.tellg();
		iss >> cmd;
		if (cmd == "today" || cmd == "yesterday" || cmd == "t" || cmd == "y") {
			cmd = "history";
			iss.seekg(pos);
		}

		std::cout << "====================" << std::endl;

		if (cmd == "add" || cmd == "a") {
			std::string name;
			double points;
			int lim_int;
			int daily_lim = 0;
			int timer_based = 0;
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
			if (iss >> daily_lim) {
				iss >> timer_based;
			}
			task_manager.add_task(Task(name, points, limited, daily_lim, timer_based));
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
			if (it == tasks.end()) {
				std::cout << "Task not found." << std::endl;
				continue;
			}
			if (it->timer_based) {
				std::cout << "Use start for timer based tasks" << std::endl;
				continue;
			}
			for (int i = 0; i < times_completed; i++) tracker.complete(*it, date);
		} else if (cmd == "start" || cmd == "s") {
			std::string name;
			if (!(iss >> name)) {
				std::cout << "Expected name" << std::endl;
				continue;
			}
			auto tasks = task_manager.get_tasks();
			auto it = std::find_if(tasks.begin(), tasks.end(), [&name](const Task& t) { return t.name == name; });
			auto date = get_current_date();

			if (it == tasks.end()) {
				std::cout << "Task not found." << std::endl;
				continue;
			}
			if (!it->timer_based) {
				std::cout << "Task " << it->name << " is not timer based" << std::endl;
				continue;
			}
			auto task_start = std::chrono::system_clock::now();
			std::cout << "Starting task " << it->name << " at " << it->points << " points/minute" << std::endl;
			std::cout << "Press enter to stop task.." << std::endl;
			std::getline(std::cin, line);
			auto task_end = std::chrono::system_clock::now();
			int minutes = std::chrono::duration_cast<std::chrono::minutes>(task_end - task_start).count();
			std::cout << minutes << " minutes completed." << std::endl;
			for (int i = 0; i < minutes; i++) tracker.complete(*it, date);
		} else if (cmd == "history" || cmd =="h") {
			std::string date_str;
			if (!(iss >> date_str)) {
				std::cout << "Expected date" << std::endl;
				continue;
			}
			if (date_str == "today" || date_str == "t") date_str = get_current_date();
			else if (date_str == "yesterday" || date_str == "y") {
				auto now = std::chrono::system_clock::now();
				auto yesterday = now - std::chrono::hours(24);
				auto in_time_t = std::chrono::system_clock::to_time_t(yesterday);
				std::tm tm = *std::localtime(&in_time_t);
				std::stringstream ss;
				ss << std::put_time(&tm, "%Y-%m-%d");
				date_str = ss.str();
			}
			tracker.view_history(date_str);
		} else if (cmd == "list" || cmd == "ls" || cmd == "l") {
			std::string filter;
			if (iss >> filter) {
				if (filter == "t" || filter == "time" || filter == "timer") filter = "timer";
				else if (filter == "l" || filter == "lim" || filter == "limit" || filter == "limited") filter = "limited";
				else if (filter == "nt" || filter == "notime" || filter == "notimer") filter = "notimer";
				else if (filter == "nl" || filter == "nolim" || filter == "nolimit" || filter == "notlimited") filter = "nolimit";
				else {
					std::cout << "Filter " << filter << " not recognized" << std::endl;
					continue;
				}
			} else {
				filter = "none";
			}
			std::cout << "Tasks:" << std::endl;
			for (const auto& t : task_manager.get_tasks()) {
				if (filter == "timer" && !t.timer_based) continue;
				if (filter == "notimer" && t.timer_based) continue;
				if (filter == "limited" && !t.is_limited) continue;
				if (filter == "nolimit" && t.is_limited) continue;
				std::cout << "    " << t.name << " (" << t.points << " pts)";
				if (t.is_limited) {
					std::cout << " [limited: " << t.daily_limit << "/day";
					auto date = get_current_date();
					int completions = tracker.completions_day(t, date);
					if (completions) {
						std::cout << ", " << t.daily_limit - completions << " left";
					}
					std::cout << "]";
				}
				if (t.timer_based) std::cout << " [timer based]";
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
