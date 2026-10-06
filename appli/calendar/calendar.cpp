#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Date {
	int year;
	int month;
	int day;
};

struct Event {
	long long dayNumber;
	string date;
	string label;
	string color;
};

string trim(const string& value) {
	size_t first = value.find_first_not_of(" \t\r\n");
	if (first == string::npos) return "";
	size_t last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

bool isLeapYear(int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int daysInMonth(int year, int month) {
	static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month == 2 && isLeapYear(year)) return 29;
	return days[month - 1];
}

bool parseDate(const string& value, Date& date) {
	if (value.size() != 10 || value[2] != '/' || value[5] != '/') return false;
	for (size_t index = 0; index < value.size(); ++index) {
		if (index != 2 && index != 5 && (value[index] < '0' || value[index] > '9')) return false;
	}
	date.day = atoi(value.substr(0, 2).c_str());
	date.month = atoi(value.substr(3, 2).c_str());
	date.year = atoi(value.substr(6, 4).c_str());
	return date.year >= 1 && date.month >= 1 && date.month <= 12 &&
		date.day >= 1 && date.day <= daysInMonth(date.year, date.month);
}

bool isCssColor(const string& value) {
	if (value.empty()) return false;
	if (value[0] == '#') {
		if (value.size() != 4 && value.size() != 5 && value.size() != 7 && value.size() != 9) return false;
		for (size_t index = 1; index < value.size(); ++index) {
			char character = value[index];
			if (!((character >= '0' && character <= '9') ||
				(character >= 'a' && character <= 'f') ||
				(character >= 'A' && character <= 'F'))) return false;
		}
		return true;
	}
	for (size_t index = 0; index < value.size(); ++index) {
		char character = value[index];
		if (!((character >= 'a' && character <= 'z') ||
			(character >= 'A' && character <= 'Z'))) return false;
	}
	return true;
}

// Number of days since a fixed civil date. This avoids timezone and DST issues.
long long toDayNumber(const Date& date) {
	int year = date.year - (date.month <= 2);
	long long era = (year >= 0 ? year : year - 399) / 400;
	unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
	unsigned shiftedMonth = static_cast<unsigned>(date.month + (date.month > 2 ? -3 : 9));
	unsigned dayOfYear = (153 * shiftedMonth + 2) / 5 + date.day - 1;
	unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
	return era * 146097 + static_cast<long long>(dayOfEra);
}

Date fromDayNumber(long long dayNumber) {
	long long era = (dayNumber >= 0 ? dayNumber : dayNumber - 146096) / 146097;
	unsigned dayOfEra = static_cast<unsigned>(dayNumber - era * 146097);
	unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
	int year = static_cast<int>(yearOfEra) + static_cast<int>(era) * 400;
	unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
	unsigned monthPart = (5 * dayOfYear + 2) / 153;
	Date date;
	date.day = static_cast<int>(dayOfYear - (153 * monthPart + 2) / 5 + 1);
	date.month = static_cast<int>(monthPart + (monthPart < 10 ? 3 : -9));
	date.year = year + (date.month <= 2);
	return date;
}

// Monday is 0, Sunday is 6.
int weekday(long long dayNumber) {
	int result = static_cast<int>((dayNumber + 2) % 7);
	if (result < 0) result += 7;
	return result;
}

string dateToString(const Date& date) {
	ostringstream output;
	if (date.day < 10) output << '0';
	output << date.day << '/';
	if (date.month < 10) output << '0';
	output << date.month << '/' << date.year;
	return output.str();
}

string htmlEscape(const string& value) {
	string escaped;
	for (size_t index = 0; index < value.size(); ++index) {
		switch (value[index]) {
		case '&': escaped += "&amp;"; break;
		case '<': escaped += "&lt;"; break;
		case '>': escaped += "&gt;"; break;
		case '"': escaped += "&quot;"; break;
		default: escaped += value[index];
		}
	}
	return escaped;
}

string dataFilePath(const string& directory, const string& filename) {
	char last = directory[directory.size() - 1];
	return directory + (last == '/' || last == '\\' ? "" : "/") + filename;
}

bool readEvents(const string& filename, vector<Event>& events) {
	ifstream input(filename.c_str());
	if (!input) {
		cerr << "Erreur: impossible d'ouvrir " << filename << endl;
		return false;
	}

	string line;
	while (getline(input, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
		if (trim(line).empty()) continue;
		size_t separator = line.find_first_of(",;\t");
		if (separator == string::npos) continue;
		string dateText = trim(line.substr(0, separator));
		string remaining = line.substr(separator + 1);
		size_t colorSeparator = remaining.find(line[separator]);
		string label = trim(remaining);
		string color;
		if (colorSeparator != string::npos) {
			string candidate = trim(remaining.substr(colorSeparator + 1));
			if (isCssColor(candidate)) {
				label = trim(remaining.substr(0, colorSeparator));
				color = candidate;
			} else if (line[separator] == '\t') {
				label = trim(remaining.substr(0, colorSeparator));
				if (!candidate.empty()) {
					cerr << "Avertissement: couleur ignoree dans event.csv: " << candidate << endl;
				}
			}
		}
		string normalizedDate = dateText;
		transform(normalizedDate.begin(), normalizedDate.end(), normalizedDate.begin(),
			[](unsigned char character) { return static_cast<char>(tolower(character)); });
		if (normalizedDate == "date") continue;
		Date date;
		if (!parseDate(dateText, date)) {
			cerr << "Avertissement: date ignoree dans event.csv: " << dateText << endl;
			continue;
		}
		if (label.empty()) continue;
		events.push_back(Event{ toDayNumber(date), dateText, label, color });
	}
	return true;
}

bool createCalendar(const Date& firstDate, const Date& lastDate, const vector<Event>& events, const string& filename) {
	long long firstDay = toDayNumber(firstDate);
	long long lastDay = toDayNumber(lastDate);
	long long firstMonday = firstDay - weekday(firstDay);
	long long lastMonday = lastDay - weekday(lastDay);
	ofstream output(filename.c_str());
	if (!output) {
		cerr << "Erreur: impossible de creer " << filename << endl;
		return false;
	}

	static const char* dayNames[] = { "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche" };
	output << "<!doctype html>\n<html lang=\"fr\">\n<head>\n<meta charset=\"utf-8\">\n";
	output << "<title>Calendrier " << dateToString(firstDate) << " - " << dateToString(lastDate) << "</title>\n";
	output << "<style>body{font-family:Arial,sans-serif;margin:2em;background:#f4f1ea;color:#292722}h1{font-size:1.5em}.filter{margin:0 0 1em}.filter input{margin-left:.5em;padding:.35em}table{border-collapse:collapse;width:100%;table-layout:fixed;background:white}th,td{border:1px solid #b9b2a5;padding:.45em;vertical-align:top}th{background:#343b3f;color:white}td{height:5em}.weekend{background:#e8f4ff}.outside{background:#eeeae2;color:#999}.outside.weekend{background:#e1edf7}.date{font-weight:bold}.event{margin-top:.35em;padding:.25em;background:#e6b85c;overflow-wrap:anywhere}</style>\n</head>\n<body>\n";
	output << "<h1>Calendrier du " << htmlEscape(dateToString(firstDate)) << " au " << htmlEscape(dateToString(lastDate)) << "</h1>\n";
	output << "<div class=\"filter\"><label for=\"event-filter\">Filtrer les événements par début de libellé :</label><input id=\"event-filter\" type=\"search\" autocomplete=\"off\"></div>\n<table>\n<tr>";
	for (int day = 0; day < 7; ++day) output << "<th>" << dayNames[day] << "</th>";
	output << "</tr>\n";

	for (long long monday = firstMonday; monday <= lastMonday; monday += 7) {
		output << "<tr>\n";
		for (int day = 0; day < 7; ++day) {
			long long currentDay = monday + day;
			Date date = fromDayNumber(currentDay);
			bool inRange = currentDay >= firstDay && currentDay <= lastDay;
			bool isWeekend = day >= 5;
			output << "<td class=\"" << (isWeekend ? "weekend" : "") << (!inRange ? " outside" : "") << "\"><div class=\"date\">" << dateToString(date) << "</div>";
			if (inRange) {
				for (size_t eventIndex = 0; eventIndex < events.size(); ++eventIndex) {
					if (events[eventIndex].dayNumber == currentDay) {
						output << "<div class=\"event\"";
						if (!events[eventIndex].color.empty()) {
							output << " style=\"background-color:" << htmlEscape(events[eventIndex].color) << "\"";
						}
						output << ">" << htmlEscape(events[eventIndex].label) << "</div>";
					}
				}
			}
			output << "</td>\n";
		}
		output << "</tr>\n";
	}
	output << "</table>\n";
	output << "<script>\n";
	output << "const eventFilter = document.getElementById('event-filter');\n";
	output << "eventFilter.focus();\n";
	output << "eventFilter.addEventListener('input', function () {\n";
	output << "  const prefix = eventFilter.value.trim().toLocaleLowerCase('fr');\n";
	output << "  document.querySelectorAll('.event').forEach(function (event) {\n";
	output << "    event.hidden = !event.textContent.trim().toLocaleLowerCase('fr').startsWith(prefix);\n";
	output << "  });\n";
	output << "});\n";
	output << "</script>\n</body>\n</html>\n";
	return true;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		cerr << "Usage: calendar date1 date2 repertoire_data (dates au format jj/mm/aaaa)" << endl;
		return 1;
	}
	Date firstDate, lastDate;
	if (!parseDate(argv[1], firstDate) || !parseDate(argv[2], lastDate)) {
		cerr << "Erreur: les dates doivent etre au format jj/mm/aaaa." << endl;
		return 1;
	}
	if (toDayNumber(firstDate) > toDayNumber(lastDate)) {
		cerr << "Erreur: date1 doit etre anterieure ou egale a date2." << endl;
		return 1;
	}

	string dataDirectory = argv[3];
	if (dataDirectory.empty()) {
		cerr << "Erreur: le repertoire de donnees ne doit pas etre vide." << endl;
		return 1;
	}
	string eventPath = dataFilePath(dataDirectory, "event.csv");
	string calendarPath = dataFilePath(dataDirectory, "calendar.html");
	vector<Event> events;
	if (!readEvents(eventPath, events)) return 1;
	if (!createCalendar(firstDate, lastDate, events, calendarPath)) return 1;
	cout << calendarPath << " cree." << endl;
	return 0;
}
