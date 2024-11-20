// banking_system.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <iomanip>

using namespace std;

// Forward declaration
class BankingSystem;

class Transaction {
private:
    string date;
    string type;
    double amount;
    string description;
    double balanceAfter;

public:
    Transaction(string t, double amt, string desc, double bal) {
        // Get current date and time
        time_t now = time(0);
        char* dt = ctime(&now);
        date = string(dt);
        date = date.substr(0, date.length()-1); // Remove newline
        type = t;
        amount = amt;
        description = desc;
        balanceAfter = bal;
    }

    // Getters for transaction details
    string getDate() const { return date; }
    string getType() const { return type; }
    double getAmount() const { return amount; }
    string getDescription() const { return description; }
    double getBalanceAfter() const { return balanceAfter; }

    // For file operations
    string toString() const {
        return date + "," + type + "," + to_string(amount) + "," +
               description + "," + to_string(balanceAfter);
    }
};

class Account {
private:
    int accountNumber;
    string name;
    string password;
    double balance;
    vector<Transaction> transactionHistory;

public:
    Account(int accNum, string n, string pass, double bal = 0.0) :
        accountNumber(accNum), name(n), password(pass), balance(bal) {}

    // Getters
    int getAccountNumber() const { return accountNumber; }
    string getName() const { return name; }
    double getBalance() const { return balance; }

    bool checkPassword(const string& pass) const {
        return password == pass;
    }

    bool deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            transactionHistory.push_back(
                Transaction("CREDIT", amount, "Deposit", balance)
            );
            return true;
        }
        return false;
    }

    bool withdraw(double amount) {
        if (amount > 0 && amount <= balance) {
            balance -= amount;
            transactionHistory.push_back(
                Transaction("DEBIT", amount, "Withdrawal", balance)
            );
            return true;
        }
        return false;
    }

    void displayTransactionHistory() const {
        cout << "\n=== Transaction History for Account " << accountNumber << " ===\n";
        cout << fixed << setprecision(2);
        for (const auto& trans : transactionHistory) {
            cout << "Date: " << trans.getDate() << endl;
            cout << "Type: " << trans.getType() << endl;
            cout << "Amount: $" << trans.getAmount() << endl;
            cout << "Description: " << trans.getDescription() << endl;
            cout << "Balance after: $" << trans.getBalanceAfter() << endl;
            cout << string(50, '-') << endl;
        }
    }

    // Save account data to file
    void saveToFile(ofstream& file) const {
        file << accountNumber << "," << name << "," << password << "," << balance << endl;
        // Save transactions
        file << transactionHistory.size() << endl;
        for (const auto& trans : transactionHistory) {
            file << trans.toString() << endl;
        }
    }

    // Add a transaction for transfers
    void addTransferTransaction(bool isCredit, double amount, int otherAccNum) {
        string type = isCredit ? "CREDIT" : "DEBIT";
        string desc = isCredit ?
            "Transfer from " + to_string(otherAccNum) :
            "Transfer to " + to_string(otherAccNum);
        transactionHistory.push_back(Transaction(type, amount, desc, balance));
    }
};

class BankingSystem {
private:
    map<int, Account> accounts;
    int nextAccountNumber;
    const string FILENAME = "bank_data.txt";

public:
    BankingSystem() : nextAccountNumber(1001) {
        loadAccounts();
    }

    void loadAccounts() {
        ifstream file(FILENAME);
        if (!file) {
            cout << "No existing account data found. Starting fresh!\n";
            return;
        }

        file >> nextAccountNumber;
        int numAccounts;
        file >> numAccounts;
        file.ignore(); // Consume newline

        for (int i = 0; i < numAccounts; i++) {
            string line;
            getline(file, line);

            // Parse account data
            size_t pos = 0;
            vector<string> tokens;
            while ((pos = line.find(",")) != string::npos) {
                tokens.push_back(line.substr(0, pos));
                line.erase(0, pos + 1);
            }
            tokens.push_back(line);

            if (tokens.size() == 4) {
                int accNum = stoi(tokens[0]);
                string name = tokens[1];
                string pass = tokens[2];
                double bal = stod(tokens[3]);

                accounts.emplace(accNum, Account(accNum, name, pass, bal));
            }
        }

        cout << "Accounts loaded successfully!\n";
        file.close();
    }

    void saveAccounts() {
        ofstream file(FILENAME);
        if (!file) {
            cout << "Error: Unable to save accounts!\n";
            return;
        }

        file << nextAccountNumber << endl;
        file << accounts.size() << endl;

        for (const auto& pair : accounts) {
            pair.second.saveToFile(file);
        }

        file.close();
    }

    pair<int, string> createAccount(const string& name, const string& password, double initialDeposit) {
        if (initialDeposit < 0) {
            return {0, "Initial deposit cannot be negative"};
        }

        int accountNum = nextAccountNumber++;
        accounts.emplace(accountNum, Account(accountNum, name, password, initialDeposit));
        saveAccounts();
        return {accountNum, "Account created successfully!"};
    }

    bool login(int accountNum, const string& password) {
        auto it = accounts.find(accountNum);
        if (it != accounts.end() && it->second.checkPassword(password)) {
            return true;
        }
        return false;
    }

    Account* getAccount(int accountNum) {
        auto it = accounts.find(accountNum);
        if (it != accounts.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    pair<bool, string> transfer(int fromAccNum, int toAccNum, double amount) {
        auto fromAcc = getAccount(fromAccNum);
        auto toAcc = getAccount(toAccNum);

        if (!fromAcc || !toAcc) {
            return {false, "Invalid account number(s)"};
        }

        if (amount <= 0) {
            return {false, "Invalid amount"};
        }

        if (fromAcc->getBalance() < amount) {
            return {false, "Insufficient balance"};
        }

        if (fromAcc->withdraw(amount)) {
            toAcc->deposit(amount);
            fromAcc->addTransferTransaction(false, amount, toAccNum);
            toAcc->addTransferTransaction(true, amount, fromAccNum);
            saveAccounts();
            return {true, "Transfer successful"};
        }

        return {false, "Transfer failed"};
    }
};

int main() {
    BankingSystem bank;
    Account* currentAccount = nullptr;

    while (true) {
        if (!currentAccount) {
            cout << "\n=== Banking System ===\n"
                 << "1. Create Account\n"
                 << "2. Login\n"
                 << "3. Exit\n"
                 << "Enter choice (1-3): ";

            int choice;
            cin >> choice;
            cin.ignore();

            switch (choice) {
                case 1: {
                    string name, password;
                    double initialDeposit;

                    cout << "Enter your name: ";
                    getline(cin, name);
                    cout << "Create password: ";
                    getline(cin, password);
                    cout << "Enter initial deposit amount: ";
                    cin >> initialDeposit;

                    auto [accountNum, message] = bank.createAccount(name, password, initialDeposit);
                    if (accountNum != 0) {
                        cout << "\n=== Account Created Successfully ===\n";
                        cout << "Your account details:\n";
                        cout << "Account Number: " << accountNum << "\n";
                        cout << "Name: " << name << "\n";
                        cout << "Initial Balance: $" << fixed << setprecision(2) << initialDeposit << "\n";
                        cout << "\nPLEASE SAVE YOUR ACCOUNT NUMBER FOR FUTURE LOGIN!\n";
                        cout << string(40, '=') << endl;
                    } else {
                        cout << "Error: " << message << endl;
                    }
                    break;
                }

                case 2: {
                    int accNum;
                    string password;

                    cout << "Enter account number: ";
                    cin >> accNum;
                    cin.ignore();
                    cout << "Enter password: ";
                    getline(cin, password);

                    if (bank.login(accNum, password)) {
                        currentAccount = bank.getAccount(accNum);
                        cout << "Login successful!\n";
                    } else {
                        cout << "Invalid credentials!\n";
                    }
                    break;
                }

                case 3:
                    cout << "Thank you for using our banking system!\n";
                    return 0;

                default:
                    cout << "Invalid choice!\n";
            }
        } else {
            cout << "\n=== Account Menu ===\n"
                 << "1. Check Balance\n"
                 << "2. Deposit\n"
                 << "3. Withdraw\n"
                 << "4. Transfer Money\n"
                 << "5. Transaction History\n"
                 << "6. Logout\n"
                 << "Enter choice (1-6): ";

            int choice;
            cin >> choice;

            switch (choice) {
                case 1:
                    cout << fixed << setprecision(2);
                    cout << "Current balance: $" << currentAccount->getBalance() << endl;
                    break;

                case 2: {
                    double amount;
                    cout << "Enter amount to deposit: ";
                    cin >> amount;
                    if (currentAccount->deposit(amount)) {
                        bank.saveAccounts();
                        cout << "Deposit successful!\n";
                    } else {
                        cout << "Invalid amount!\n";
                    }
                    break;
                }

                case 3: {
                    double amount;
                    cout << "Enter amount to withdraw: ";
                    cin >> amount;
                    if (currentAccount->withdraw(amount)) {
                        bank.saveAccounts();
                        cout << "Withdrawal successful!\n";
                    } else {
                        cout << "Invalid amount or insufficient balance!\n";
                    }
                    break;
                }

                case 4: {
                    int toAccount;
                    double amount;
                    cout << "Enter recipient's account number: ";
                    cin >> toAccount;
                    cout << "Enter amount to transfer: ";
                    cin >> amount;

                    auto [success, message] = bank.transfer(
                        currentAccount->getAccountNumber(),
                        toAccount,
                        amount
                    );
                    cout << message << endl;
                    break;
                }

                case 5:
                    currentAccount->displayTransactionHistory();
                    break;

                case 6:
                    currentAccount = nullptr;
                    cout << "Logged out successfully!\n";
                    break;

                default:
                    cout << "Invalid choice!\n";
            }
        }
    }

    return 0;
}
