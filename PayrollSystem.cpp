#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;

// =============== Namespaces for Configurations ===============
// Contains all payroll calculation constants and settings
namespace Payroll {
    const double TAX_FREE_ALLOWANCE = 12570.0;  // UK tax-free allowance
    const double TAX_RATE = 0.20;               // 20% tax rate
    const int MONTHS_IN_YEAR = 12;
    const int SORT_HOURLY_RATE = 1;
    const int SORT_HOURS_WORKED = 2;
    const int SORT_NET_PAY = 3;
    const int FILE_CHOICE_SAME = 1;
    const int FILE_CHOICE_NEW = 2;
}

// Menu option constants to avoid magic numbers
namespace Menu {
    const int QUIT = 0;
    const int PROCESS_PAY_FILE = 1;
    const int VIEW_ALL_SALARY = 2;
    const int VIEW_INDIVIDUAL = 3;
    const int SORT_EMPLOYEES = 4;
    const int VIEW_EMPLOYEE_TOTALS = 5;
    const int INVALID_CHOICE = -1;
}

// Input handling limits
namespace Limits {
    const int CIN_IGNORE_LIMIT = 1000;
}

// File naming conventions
namespace FileNames {
    const string EMPLOYEES_FILE = "employees.txt";
    const string ERROR_LOG_FILE = "errors.txt";
    const string OUTPUT_SUFFIX = "_output.txt";
}

namespace FileExt {
    const string TXT = ".txt";
}

// User input constants
namespace Inputs {
    const char YES = 'y';
    const char NO = 'n';
    const string RETURN = "0";
}

const string CURRENCY = "£";

// =============== Utility Functions ===============
// Removes whitespace from beginning and end of string
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Converts string to uppercase
string toUpper(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::toupper);
    return out;
}

// Converts string to lowercase
string toLower(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// =============== Employee Class ===============
class Employee {
public:
    string id;
    string name;
    double hourlyRate;
    map<string, double> hoursWorked;  // Maps month to hours worked

    Employee() : hourlyRate(0.0) {}
    Employee(const string& _id, const string& _name, double _rate)
        : id(_id), name(_name), hourlyRate(_rate) {}

    // Calculate gross pay for a specific month
    double getGrossPay(const string& month) const {
        auto it = hoursWorked.find(month);
        if (it == hoursWorked.end()) return 0.0;
        return hourlyRate * it->second;
    }

    // Calculate monthly tax based on annual projection
    double getTax(const string& month) const {
        double gross = getGrossPay(month);
        double annual = gross * Payroll::MONTHS_IN_YEAR;  // Project annual salary
        double taxable = annual - Payroll::TAX_FREE_ALLOWANCE;
        if (taxable < 0) taxable = 0;
        double annualTax = taxable * Payroll::TAX_RATE;
        return annualTax / Payroll::MONTHS_IN_YEAR;  // Return monthly portion
    }

    // Calculate net pay after tax deduction
    double getNetPay(const string& month) const {
        return getGrossPay(month) - getTax(month);
    }

    // Calculate total gross pay across all months
    double getTotalGross() const {
        double total = 0.0;
        for (const auto& rec : hoursWorked)
            total += getGrossPay(rec.first);
        return total;
    }

    // Calculate total tax across all months
    double getTotalTax() const {
        double total = 0.0;
        for (const auto& rec : hoursWorked)
            total += getTax(rec.first);
        return total;
    }

    // Calculate total net pay across all months
    double getTotalNet() const {
        double total = 0.0;
        for (const auto& rec : hoursWorked)
            total += getNetPay(rec.first);
        return total;
    }
};

// =============== PayrollSystem Class ===============
class PayrollSystem {
private:
    map<string, Employee> employees;     // Employee database
    set<string> loadedPayFiles;          // Track processed files to avoid duplicates
    vector<string> processedMonths;      // Keep order of processed months
    vector<pair<string, string>> errors; // Store errors for logging

    // Display formatting constants
    static const int HEADER_TOTAL_WIDTH = 70;
    static const int LINE_TOTAL_WIDTH = 50;

    // Print separator lines for better formatting
    void printLine(int totalWidth) {
        cout << setfill('=') << setw(totalWidth) << "" << setfill(' ') << endl;
    }
    void printShortLine(int totalWidth) {
        cout << setfill('-') << setw(totalWidth) << "" << setfill(' ') << endl;
    }

    // Print properly aligned table headers
    void printAlignedHeader(std::ostream& out) const {
        // Column widths - these should match the data widths exactly
        const int w_id    = 8;
        const int w_name  = 18;
        const int w_rate  = 11;
        const int w_hours = 8;
        const int w_gross = 13;
        const int w_tax   = 12;
        const int w_net   = 13;

        out << left << setw(w_id) << "ID"
            << left << setw(w_name) << "Name"
            << right << setw(w_rate) << "Rate(£)"
            << right << setw(w_hours) << "Hours"
            << right << setw(w_gross) << "Gross(£)"
            << right << setw(w_tax) << "Tax(£)"
            << right << setw(w_net) << "Net(£)"
            << endl;
    }

    // Robust input validation for menu selections
    int getIntInput(int min, int max, const string& prompt) const {
        int val;
        while (true) {
            cout << prompt;
            if (cin >> val) {
                cin.ignore(Limits::CIN_IGNORE_LIMIT, '\n');
                if (val >= min && val <= max) return val;
                cout << "Invalid input. Please enter a number between " << min << " and " << max << ".\n";
            } else {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a valid number.\n";
            }
        }
    }

    // Get string input with prompt
    string getStringInput(const string& prompt) const {
        string input;
        cout << prompt;
        getline(cin, input);
        return input;
    }

    // Get validated yes/no input
    char getYesNoInput(const string& prompt) const {
        char choice;
        while (true) {
            cout << prompt;
            if (cin >> choice) {
                cin.ignore(Limits::CIN_IGNORE_LIMIT, '\n');
                choice = static_cast<char>(tolower(choice));
                if (choice == Inputs::YES || choice == Inputs::NO) return choice;
                cout << "Invalid input. Please enter 'y' or 'n'.\n";
            } else {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter 'y' or 'n'.\n";
            }
        }
    }

public:
    PayrollSystem() = default;

    // Load employee master data from file
    bool loadEmployees(const string& filename) {
        ifstream fin(filename);
        if (!fin) {
            cerr << "Error: Could not open " << filename << endl;
            return false;
        }
        string line;
        while (getline(fin, line)) {
            istringstream iss(line);
            string id, name;
            double rate;
            if (!(iss >> id >> name >> rate)) continue;  // Skip malformed lines
            id = toUpper(trim(id));
            name = trim(name);
            employees[id] = Employee(id, name, rate);
        }
        fin.close();
        return true;
    }

    // Load pay file with hours worked for specific month
    bool loadPayFile(const string& filename, string& outMonth, bool replace = false) {
        // Extract month from filename (e.g., "jan25.txt" -> "JAN25")
        string month = filename.substr(0, filename.find(FileExt::TXT));
        string upMonth = toUpper(month);
        outMonth = upMonth;

        // Check for duplicate processing
        if (loadedPayFiles.count(upMonth) && !replace) {
            char choice = getYesNoInput("This file has already been processed.\nDo you want to replace it? (y/n): ");
            if (choice == Inputs::YES) {
                removePayRecordsForMonth(upMonth);
                loadedPayFiles.erase(upMonth);
            } else {
                return false;
            }
        }

        ifstream fin(filename);
        if (!fin) {
            string err = "Pay file " + filename + " could not be found.";
            errors.push_back({filename, err});
            cerr << err << endl;
            logErrors();
            return false;
        }

        // Process each line: employee_id hours_worked
        string line;
        while (getline(fin, line)) {
            istringstream iss(line);
            string id;
            double hours;
            if (!(iss >> id >> hours)) continue;  // Skip malformed lines
            id = toUpper(trim(id));
            if (employees.count(id))
                employees[id].hoursWorked[upMonth] = hours;
            else
                errors.push_back({filename, id + " is not a valid employee ID number."});
        }

        loadedPayFiles.insert(upMonth);
        processedMonths.push_back(upMonth);
        fin.close();
        logErrors();
        return true;
    }

    // Remove pay records for a specific month (used when replacing data)
    void removePayRecordsForMonth(const string& month) {
        for (auto& pair : employees)
            pair.second.hoursWorked.erase(month);
        auto it = find(processedMonths.begin(), processedMonths.end(), month);
        if (it != processedMonths.end()) processedMonths.erase(it);
    }

    // Write payroll summary to output file
    void writeMonthOutput(const string& month) {
        string fname = toLower(month) + FileNames::OUTPUT_SUFFIX;
        ofstream fout(fname);
        if (!fout) {
            cerr << "Error: Cannot write to " << fname << endl;
            return;
        }

        // Column width constants for consistent formatting
        const int w_id    = 8;
        const int w_name  = 18;
        const int w_rate  = 10;
        const int w_hours = 8;
        const int w_gross = 12;
        const int w_tax   = 10;
        const int w_net   = 12;

        // Write header
        printAlignedHeader(fout);

        // Write employee data for this month
        for (const auto& pair : employees) {
            const Employee& e = pair.second;
            if (e.hoursWorked.count(month)) {
                fout << left << setw(w_id) << e.id
                     << left << setw(w_name) << e.name
                     << right << setw(w_rate) << fixed << setprecision(2) << e.hourlyRate
                     << right << setw(w_hours) << fixed << setprecision(2) << e.hoursWorked.at(month)
                     << right << setw(w_gross) << fixed << setprecision(2) << e.getGrossPay(month)
                     << right << setw(w_tax) << fixed << setprecision(2) << e.getTax(month)
                     << right << setw(w_net) << fixed << setprecision(2) << e.getNetPay(month) << endl;
            }
        }
        fout.close();
        cout << "Wrote pay details to " << fname << endl;
    }

    // Write errors to log file
    void logErrors() {
        if (errors.empty()) return;
        ofstream fout(FileNames::ERROR_LOG_FILE, ios::app);
        for (const auto& err : errors)
            fout << err.first << "\n" << err.second << "\n";
        fout.close();
        errors.clear();
    }

    // =============== Organized Display Functions ===============
    // Display payroll summary for a specific month
    void printMonthSummary(const string& month) {
        const int w_id    = 8;
        const int w_name  = 18;
        const int w_rate  = 10;
        const int w_hours = 8;
        const int w_gross = 12;
        const int w_tax   = 10;
        const int w_net   = 12;

        cout << "\n";
        printLine(HEADER_TOTAL_WIDTH);
        cout << "Monthly Summary: " << month << "\n";
        printShortLine(HEADER_TOTAL_WIDTH);
        printAlignedHeader(cout);
        printShortLine(HEADER_TOTAL_WIDTH);

        // Display each employee who worked this month
        for (const auto& [_, emp] : employees) {
            if (emp.hoursWorked.count(month)) {
                cout << left << setw(w_id) << emp.id
                     << left << setw(w_name) << emp.name
                     << right << setw(w_rate) << fixed << setprecision(2) << emp.hourlyRate
                     << right << setw(w_hours) << fixed << setprecision(2) << emp.hoursWorked.at(month)
                     << right << setw(w_gross) << fixed << setprecision(2) << emp.getGrossPay(month)
                     << right << setw(w_tax) << fixed << setprecision(2) << emp.getTax(month)
                     << right << setw(w_net) << fixed << setprecision(2) << emp.getNetPay(month) << endl;
            }
        }
        printLine(HEADER_TOTAL_WIDTH);
    }

    // Show employee selection menu for detailed breakdown
    void showEmployeeBreakdown() {
        vector<string> idList;
        int count = 1;
        printShortLine(LINE_TOTAL_WIDTH);
        cout << "Select Employee\n";
        printShortLine(LINE_TOTAL_WIDTH);

        // Build numbered list of employees
        for (const auto& [id, emp] : employees) {
            cout << setw(3) << count << ". " << emp.id << " (" << emp.name << ")\n";
            idList.push_back(id);
            ++count;
        }
        printShortLine(LINE_TOTAL_WIDTH);

        int sel = getIntInput(0, static_cast<int>(idList.size()), "Select employee by number (or 0 to return): ");
        if (sel <= 0 || sel > static_cast<int>(idList.size())) return;
        displayEmployeeDetails(idList[sel - 1]);
    }

    // Display detailed breakdown for individual employee
    void displayEmployeeDetails(const string& empid) {
        auto it = employees.find(empid);
        if (it == employees.end()) {
            cout << "Error: The selected employee does not exist in the records.\n";
            return;
        }

        const Employee& e = it->second;
        // Column widths for employee detail table
        const int w_month = 12;
        const int w_hours = 8;
        const int w_gross = 19;
        const int w_tax = 22;
        const int w_net = 25;

        cout << "\n";
        printLine(60);
        cout << "Details for " << e.id << " (" << e.name << ")\n";
        printShortLine(60);

        // Table header
        cout << left  << setw(w_month) << "Month"
             << right << setw(w_hours) << "Hours"
             << right << setw(w_gross) << "Gross(£)"
             << right << setw(w_tax)   << "Tax(£)"
             << right << setw(w_net)   << "Net(£)" << endl;
        printShortLine(60);

        // Display each month's data and calculate totals
        double totalGross = 0, totalTax = 0, totalNet = 0;
        for (const auto& rec : e.hoursWorked) {
            const string& month = rec.first;
            double hours = rec.second;
            cout << left  << setw(w_month) << month
                 << right << setw(w_hours) << fixed << setprecision(2) << hours
                 << right << setw(w_gross) << CURRENCY << fixed << setprecision(2) << e.getGrossPay(month)
                 << right << setw(w_tax)   << CURRENCY << fixed << setprecision(2) << e.getTax(month)
                 << right << setw(w_net)   << CURRENCY << fixed << setprecision(2) << e.getNetPay(month) << endl;
            totalGross += e.getGrossPay(month);
            totalTax += e.getTax(month);
            totalNet += e.getNetPay(month);
        }

        // Display totals row
        printShortLine(60);
        cout << left  << setw(w_month) << "Totals:"
             << right << setw(w_hours) << "" // empty for hours column
             << right << setw(w_gross) << CURRENCY << fixed << setprecision(2) << totalGross
             << right << setw(w_tax)   << CURRENCY << fixed << setprecision(2) << totalTax
             << right << setw(w_net)   << CURRENCY << fixed << setprecision(2) << totalNet << endl;
        printLine(60);
    }

    // Display employee totals summary
    void showEmployeeTotals() {
        vector<string> idList;
        int count = 1;
        printShortLine(LINE_TOTAL_WIDTH);
        cout << "Employee List\n";
        printShortLine(LINE_TOTAL_WIDTH);

        // Build employee selection list
        for (const auto& [id, emp] : employees) {
            cout << setw(3) << count << ". " << emp.id << " (" << emp.name << ")\n";
            idList.push_back(id);
            ++count;
        }
        printShortLine(LINE_TOTAL_WIDTH);

        int sel = getIntInput(0, static_cast<int>(idList.size()), "Select employee by number (or 0 to return): ");
        if (sel <= 0 || sel > static_cast<int>(idList.size())) return;

        // Display summary totals for selected employee
        const Employee& e = employees[idList[sel - 1]];
        printLine(LINE_TOTAL_WIDTH);
        cout << "Totals for " << e.id << " (" << e.name << "):\n";
        printShortLine(LINE_TOTAL_WIDTH);
        cout << left << setw(16) << "Total Gross:" << CURRENCY << fixed << setprecision(2) << e.getTotalGross() << endl;
        cout << left << setw(16) << "Total Tax:" << CURRENCY << fixed << setprecision(2) << e.getTotalTax() << endl;
        cout << left << setw(16) << "Total Net:" << CURRENCY << fixed << setprecision(2) << e.getTotalNet() << endl;
        printLine(LINE_TOTAL_WIDTH);
    }

    // Main program loop
    void run() {
        cout << "Welcome to the Payroll System\n";
        if (!loadEmployees(FileNames::EMPLOYEES_FILE)) {
            cout << "Cannot continue without employee records.\n";
            return;
        }

        int choice = Menu::INVALID_CHOICE;
        while (choice != Menu::QUIT) {
            // Display main menu
            printLine(LINE_TOTAL_WIDTH);
            cout << "Main Menu:\n";
            printShortLine(LINE_TOTAL_WIDTH);
            cout << Menu::PROCESS_PAY_FILE << ". Process Pay File\n";
            cout << Menu::VIEW_ALL_SALARY << ". View All Salary Details\n";
            cout << Menu::VIEW_INDIVIDUAL << ". View Individual Employee Details\n";
            cout << Menu::SORT_EMPLOYEES << ". Sort Employees\n";
            cout << Menu::VIEW_EMPLOYEE_TOTALS << ". View Employee Totals\n";
            cout << Menu::QUIT << ". Quit\n";
            printShortLine(LINE_TOTAL_WIDTH);

            choice = getIntInput(Menu::QUIT, Menu::VIEW_EMPLOYEE_TOTALS, "Enter choice: ");

            // Handle menu selection
            switch (choice) {
                case Menu::PROCESS_PAY_FILE: processPayFileMenu(); break;
                case Menu::VIEW_ALL_SALARY: viewAllSalaryDetailsMenu(); break;
                case Menu::VIEW_INDIVIDUAL: showEmployeeBreakdown(); break;
                case Menu::SORT_EMPLOYEES: sortEmployeesMenu(); break;
                case Menu::VIEW_EMPLOYEE_TOTALS: showEmployeeTotals(); break;
                case Menu::QUIT: cout << "Goodbye!\n"; break;
                default: cout << "Invalid choice. Try again.\n";
            }
        }
    }

    // Menu for processing pay files
    void processPayFileMenu() {
        while (true) {
            string fname = getStringInput("Enter pay file to process (e.g., jan25.txt), or '0' to return: ");
            if (fname == Inputs::RETURN) return;
            string month;
            if (loadPayFile(fname, month)) {
                cout << "File " << fname << " processed successfully as month " << month << ".\n";
                writeMonthOutput(month);  // Create output file automatically
            }
        }
    }

    // Menu for viewing salary details by month
    void viewAllSalaryDetailsMenu() {
        if (processedMonths.empty()) {
            cout << "No pay files processed yet.\n";
            return;
        }
        while (true) {
            // Show available months
            cout << "Processed months: ";
            for (size_t i = 0; i < processedMonths.size(); ++i)
                cout << (i + 1) << "." << processedMonths[i] << " ";
            cout << "\n";

            int idx = getIntInput(0, static_cast<int>(processedMonths.size()), "Enter number to view details, or 0 to return: ");
            if (idx == 0) return;
            string month = processedMonths[idx - 1];
            printMonthSummary(month);
        }
    }

    // Menu for sorting employees by various criteria
    void sortEmployeesMenu() {
        // Column width constants
        const int w_id    = 8;
        const int w_name  = 18;
        const int w_rate  = 10;
        const int w_hours = 8;
        const int w_gross = 12;
        const int w_tax   = 10;
        const int w_net   = 12;

        if (processedMonths.empty()) {
            cout << "No pay files processed yet.\n";
            return;
        }

        // Select month to sort by
        cout << "\nSort Employees\n";
        cout << "Choose month to sort by:\n";
        for (size_t i = 0; i < processedMonths.size(); ++i)
            cout << (i + 1) << ". " << processedMonths[i] << " ";
        cout << "\n";

        int idx = getIntInput(0, static_cast<int>(processedMonths.size()), "Enter number (or 0 to return): ");
        if (idx == 0 || idx > static_cast<int>(processedMonths.size())) return;
        string month = processedMonths[idx - 1];

        // Select sorting criteria
        cout << "Sort by:\n";
        cout << Payroll::SORT_HOURLY_RATE << ". Hourly Rate\n";
        cout << Payroll::SORT_HOURS_WORKED << ". Hours Worked\n";
        cout << Payroll::SORT_NET_PAY << ". Net Pay\n";
        int crit = getIntInput(Payroll::SORT_HOURLY_RATE, Payroll::SORT_NET_PAY, "Enter choice: ");

        // Build list of employees who worked in selected month
        vector<Employee> emps;
        for (const auto& pair : employees)
            if (pair.second.hoursWorked.count(month))
                emps.push_back(pair.second);

        // Display header
        printShortLine(HEADER_TOTAL_WIDTH);
        printAlignedHeader(cout);
        printShortLine(HEADER_TOTAL_WIDTH);

        // Sort employees based on selected criteria (descending order)
        switch (crit) {
            case Payroll::SORT_HOURLY_RATE:
                sort(emps.begin(), emps.end(), [](const Employee& a, const Employee& b) {
                    return a.hourlyRate > b.hourlyRate;
                }); break;
            case Payroll::SORT_HOURS_WORKED:
                sort(emps.begin(), emps.end(), [month](const Employee& a, const Employee& b) {
                    return a.hoursWorked.at(month) > b.hoursWorked.at(month);
                }); break;
            case Payroll::SORT_NET_PAY:
                sort(emps.begin(), emps.end(), [month](const Employee& a, const Employee& b) {
                    return a.getNetPay(month) > b.getNetPay(month);
                }); break;
        }

        // Display sorted results
        for (const auto& e : emps) {
            cout << left << setw(w_id) << e.id
                 << left << setw(w_name) << e.name
                 << right << setw(w_rate) << fixed << setprecision(2) << e.hourlyRate
                 << right << setw(w_hours) << fixed << setprecision(2) << e.hoursWorked.at(month)
                 << right << setw(w_gross) << fixed << setprecision(2) << e.getGrossPay(month)
                 << right << setw(w_tax) << fixed << setprecision(2) << e.getTax(month)
                 << right << setw(w_net) << fixed << setprecision(2) << e.getNetPay(month) << endl;
        }
        printLine(HEADER_TOTAL_WIDTH);
    }
};

// =============== Program Entry Point ===============
int main() {
    PayrollSystem sys;
    sys.run();
    return 0;
}
