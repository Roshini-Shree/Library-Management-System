#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <limits>
#include <iomanip>
#include <ctime>

using namespace std;

const string DATA_FILE = "library_catalog.txt";
const int LOAN_PERIOD_DAYS = 14;

// Clears bad input from the buffer so the program doesn't get stuck
void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ===========================================================
// Base class: MediaItem
// Abstract class demonstrating inheritance + polymorphism.
// Book and Journal both derive from this and override its
// virtual methods differently (e.g. fine rate, display format).
// ===========================================================
class MediaItem {
protected:
    int id;
    string title;
    string creator;        // author (Book) or publisher (Journal)
    bool checkedOut;
    string borrowerName;
    time_t checkoutDate;

public:
    MediaItem(int itemId, string itemTitle, string itemCreator)
        : id(itemId), title(itemTitle), creator(itemCreator),
          checkedOut(false), borrowerName(""), checkoutDate(0) {}

    // Virtual destructor is required whenever a base class is used
    // polymorphically through pointers, so derived members are cleaned up
    virtual ~MediaItem() {}

    int getId() const { return id; }
    string getTitle() const { return title; }
    bool isCheckedOut() const { return checkedOut; }

    // Pure virtual methods: every derived class MUST implement these
    // in its own way -> this is the polymorphism the task asks for.
    virtual string getType() const = 0;
    virtual double getFinePerDay() const = 0;
    virtual void display() const = 0;
    virtual string serialize() const = 0;

    void checkout(const string& borrower) {
        checkedOut = true;
        borrowerName = borrower;
        checkoutDate = time(nullptr);
    }

    // daysElapsed is supplied manually at return time so fines can be
    // demonstrated/tested without waiting real days for a book to go overdue.
    double returnItem(int daysElapsed) {
        checkedOut = false;
        borrowerName = "";
        checkoutDate = 0;

        int overdueDays = daysElapsed - LOAN_PERIOD_DAYS;
        if (overdueDays <= 0) return 0.0;
        return overdueDays * getFinePerDay();
    }

    string getBorrower() const { return borrowerName; }

    // Shared part of every item's serialized line: id|title|creator|checkedOut|borrower
    string baseFields() const {
        ostringstream oss;
        oss << id << "|" << title << "|" << creator << "|"
            << (checkedOut ? 1 : 0) << "|" << borrowerName;
        return oss.str();
    }
};

// ===========================================================
// Derived class: Book
// ===========================================================
class Book : public MediaItem {
private:
    string genre;
    string isbn;

public:
    Book(int id, string title, string author, string bookGenre, string bookIsbn)
        : MediaItem(id, title, author), genre(bookGenre), isbn(bookIsbn) {}

    string getType() const override { return "Book"; }

    double getFinePerDay() const override { return 5.0; } // Rs. 5/day for books

    void display() const override {
        cout << left << setw(5) << id << setw(10) << "Book"
             << setw(25) << title << setw(20) << creator
             << setw(10) << genre
             << (checkedOut ? ("Checked out to " + borrowerName) : "Available")
             << "\n";
    }

    string serialize() const override {
        return "BOOK|" + baseFields() + "|" + genre + "|" + isbn;
    }

    static unique_ptr<Book> deserialize(int id, string title, string creator,
                                         bool checkedOut, string borrower,
                                         const string& genre, const string& isbn) {
        auto book = make_unique<Book>(id, title, creator, genre, isbn);
        if (checkedOut) {
            book->checkedOut = true;
            book->borrowerName = borrower;
        }
        return book;
    }
};

// ===========================================================
// Derived class: Journal
// ===========================================================
class Journal : public MediaItem {
private:
    string issueNumber;
    string publisher;

public:
    Journal(int id, string title, string pub, string issue)
        : MediaItem(id, title, pub), issueNumber(issue), publisher(pub) {}

    string getType() const override { return "Journal"; }

    double getFinePerDay() const override { return 2.0; } // Rs. 2/day for journals

    void display() const override {
        cout << left << setw(5) << id << setw(10) << "Journal"
             << setw(25) << title << setw(20) << creator
             << setw(10) << issueNumber
             << (checkedOut ? ("Checked out to " + borrowerName) : "Available")
             << "\n";
    }

    string serialize() const override {
        return "JOURNAL|" + baseFields() + "|" + issueNumber + "|" + publisher;
    }

    static unique_ptr<Journal> deserialize(int id, string title, string creator,
                                             bool checkedOut, string borrower,
                                             const string& issue, const string& pub) {
        auto journal = make_unique<Journal>(id, title, pub, issue);
        if (checkedOut) {
            journal->checkedOut = true;
            journal->borrowerName = borrower;
        }
        return journal;
    }
};

// ===========================================================
// Library: owns the catalog and all operations on it.
// Uses unique_ptr<MediaItem> for safe dynamic memory management --
// each item is heap-allocated once and automatically freed when
// removed from the vector or when the program ends.
// ===========================================================
class Library {
private:
    vector<unique_ptr<MediaItem>> catalog;
    int nextId;

public:
    Library() : nextId(1) {
        loadFromFile();
        for (const auto& item : catalog) {
            if (item->getId() >= nextId) nextId = item->getId() + 1;
        }
    }

    void addBook() {
        string title, author, genre, isbn;
        cin.ignore();
        cout << "Enter title: "; getline(cin, title);
        cout << "Enter author: "; getline(cin, author);
        cout << "Enter genre: "; getline(cin, genre);
        cout << "Enter ISBN: "; getline(cin, isbn);

        catalog.push_back(make_unique<Book>(nextId, title, author, genre, isbn));
        cout << "Book added with ID " << nextId << ".\n";
        nextId++;
        saveToFile();
    }

    void addJournal() {
        string title, publisher, issue;
        cin.ignore();
        cout << "Enter title: "; getline(cin, title);
        cout << "Enter publisher: "; getline(cin, publisher);
        cout << "Enter issue number: "; getline(cin, issue);

        catalog.push_back(make_unique<Journal>(nextId, title, publisher, issue));
        cout << "Journal added with ID " << nextId << ".\n";
        nextId++;
        saveToFile();
    }

    // Returns a raw observer pointer -- Library still owns the object
    MediaItem* findById(int id) {
        for (auto& item : catalog) {
            if (item->getId() == id) return item.get();
        }
        return nullptr;
    }

    void displayCatalog() const {
        if (catalog.empty()) {
            cout << "\nCatalog is empty.\n";
            return;
        }
        cout << "\n" << left << setw(5) << "ID" << setw(10) << "Type"
             << setw(25) << "Title" << setw(20) << "Author/Publisher"
             << setw(10) << "Genre/Issue" << "Status\n";
        cout << string(95, '-') << "\n";

        // Calling display() through a base-class pointer runs the
        // correct derived version automatically -- this is runtime polymorphism.
        for (const auto& item : catalog) {
            item->display();
        }
    }

    void checkoutItem() {
        int id;
        cout << "Enter item ID to checkout: ";
        cin >> id;
        if (cin.fail()) { cout << "Invalid input!\n"; clearInputBuffer(); return; }

        MediaItem* item = findById(id);
        if (!item) { cout << "No item found with ID " << id << ".\n"; return; }
        if (item->isCheckedOut()) {
            cout << "\"" << item->getTitle() << "\" is already checked out to "
                 << item->getBorrower() << ".\n";
            return;
        }

        cin.ignore();
        string borrower;
        cout << "Enter borrower name: ";
        getline(cin, borrower);

        item->checkout(borrower);
        cout << "\"" << item->getTitle() << "\" checked out to " << borrower
             << ". Due in " << LOAN_PERIOD_DAYS << " days.\n";
        saveToFile();
    }

    void returnItem() {
        int id;
        cout << "Enter item ID to return: ";
        cin >> id;
        if (cin.fail()) { cout << "Invalid input!\n"; clearInputBuffer(); return; }

        MediaItem* item = findById(id);
        if (!item) { cout << "No item found with ID " << id << ".\n"; return; }
        if (!item->isCheckedOut()) {
            cout << "\"" << item->getTitle() << "\" is not currently checked out.\n";
            return;
        }

        int daysElapsed;
        cout << "Enter number of days since checkout: ";
        cin >> daysElapsed;
        if (cin.fail() || daysElapsed < 0) {
            cout << "Invalid input!\n"; clearInputBuffer(); return;
        }

        // Polymorphic call: fine rate depends on whether it's a Book or Journal
        double fine = item->returnItem(daysElapsed);

        cout << "\"" << item->getTitle() << "\" returned successfully.\n";
        if (fine > 0) {
            cout << "Item was overdue! Fine due: Rs. " << fixed << setprecision(2) << fine << "\n";
        } else {
            cout << "Returned on time. No fine.\n";
        }
        saveToFile();
    }

    void searchByTitle() const {
        cin.ignore();
        string query;
        cout << "Enter title to search: ";
        getline(cin, query);

        bool found = false;
        for (const auto& item : catalog) {
            if (item->getTitle().find(query) != string::npos) {
                item->display();
                found = true;
            }
        }
        if (!found) cout << "No matching items found.\n";
    }

    // ---------------- File persistence ----------------
    void saveToFile() const {
        ofstream file(DATA_FILE);
        for (const auto& item : catalog) {
            file << item->serialize() << "\n";
        }
    }

    void loadFromFile() {
        ifstream file(DATA_FILE);
        string line;
        catalog.clear();

        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string type, idStr, title, creator, checkedOutStr, borrower, extra1, extra2;

            getline(ss, type, '|');
            getline(ss, idStr, '|');
            getline(ss, title, '|');
            getline(ss, creator, '|');
            getline(ss, checkedOutStr, '|');
            getline(ss, borrower, '|');
            getline(ss, extra1, '|');
            getline(ss, extra2, '|');

            int id = stoi(idStr);
            bool checkedOut = (checkedOutStr == "1");

            if (type == "BOOK") {
                catalog.push_back(Book::deserialize(id, title, creator, checkedOut,
                                                     borrower, extra1, extra2));
            } else if (type == "JOURNAL") {
                catalog.push_back(Journal::deserialize(id, title, creator, checkedOut,
                                                        borrower, extra1, extra2));
            }
        }
    }
};

// ===========================================================
// Menu-driven interface
// ===========================================================
void showMenu() {
    cout << "\n===== Library Management System =====\n";
    cout << "1. Add Book\n";
    cout << "2. Add Journal\n";
    cout << "3. View Catalog\n";
    cout << "4. Checkout Item\n";
    cout << "5. Return Item\n";
    cout << "6. Search by Title\n";
    cout << "7. Exit\n";
    cout << "======================================\n";
    cout << "Enter your choice (1-7): ";
}

int main() {
    Library library;
    int choice;
    bool running = true;

    cout << "Welcome to the Library Management System.\n";
    cout << "(Catalog is loaded from and saved to " << DATA_FILE << ")\n";

    while (running) {
        showMenu();
        cin >> choice;

        if (cin.fail()) {
            cout << "Invalid choice! Please enter a number.\n";
            clearInputBuffer();
            continue;
        }

        switch (choice) {
            case 1: library.addBook(); break;
            case 2: library.addJournal(); break;
            case 3: library.displayCatalog(); break;
            case 4: library.checkoutItem(); break;
            case 5: library.returnItem(); break;
            case 6: library.searchByTitle(); break;
            case 7:
                cout << "Saving catalog and exiting. Goodbye!\n";
                running = false;
                break;
            default:
                cout << "Invalid choice! Please select an option between 1 and 7.\n";
        }
    }

    return 0;
}
