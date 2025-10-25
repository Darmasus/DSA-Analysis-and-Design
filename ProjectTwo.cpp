/***************************************************************
    CS 300 Project Two
    Author: Alejandro Coitinho

    Program goal
    - Read course data from a CSV file
    - Store courses in a Binary Search Tree keyed by course number
    - Provide a menu to load data, print the full sorted course list,
      and print a single course with its prerequisites

      For Professor Griffith
    - This version includes move semantics to fix the memory access crash
      when transferring the tree. I had to work on this extensively after dozens of crashes.
    - The structure and naming match the approved pseudocode.
    - No external CSV libraries are used. The trade-off is you do have to input a precise file name to load the CSV.
      I opted for this because it gives you fredom to have multiple CSV within the directory to load rathern than 
      a single one.
****************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>

using namespace std;

/* -------------------------------------------------------------
   Helper trimming and case handling utilities
------------------------------------------------------------- */

static inline string ltrim(const string& s) {
    size_t i = 0;
    while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) ++i;
    return s.substr(i);
}

static inline string rtrim(const string& s) {
    if (s.empty()) return s;
    size_t j = s.size() - 1;
    while (j < s.size() && isspace(static_cast<unsigned char>(s[j]))) {
        if (j == 0) break;
        --j;
    }
    if (isspace(static_cast<unsigned char>(s[j]))) return string();
    return s.substr(0, j + 1);
}

static inline string trim(const string& s) {
    return rtrim(ltrim(s));
}

// Convert to uppercase for consistent key comparison
static inline string toUpper(const string& s) {
    string out = s;
    for (char& c : out) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return out;
}

/* -------------------------------------------------------------
   Course structure
------------------------------------------------------------- */

struct Course {
    string number;                 // ex: CS200
    string title;                  // ex: Data Structures
    vector<string> prerequisites;  // ex: CS100, MATH200
};

/* -------------------------------------------------------------
   Binary Search Tree to store Course by course number
------------------------------------------------------------- */

struct Node {
    Course data;
    Node* left;
    Node* right;

    explicit Node(const Course& c) : data(c), left(nullptr), right(nullptr) {}
};

class CourseBST {
public:
    CourseBST() : root(nullptr) {}

    // --- Move constructor (to safely transfer ownership) ---
    CourseBST(CourseBST&& other) noexcept : root(other.root) {
        other.root = nullptr;
    }

    // --- Move assignment (to prevent double deletion) ---
    CourseBST& operator=(CourseBST&& other) noexcept {
        if (this != &other) {
            destroy(root);
            root = other.root;
            other.root = nullptr;
        }
        return *this;
    }

    // Prevent shallow copies
    CourseBST(const CourseBST&) = delete;
    CourseBST& operator=(const CourseBST&) = delete;

    ~CourseBST() {
        destroy(root);
    }

    // Insert course into BST
    void insert(const Course& c) {
        root = insertRec(root, c);
    }

    // Search course by number
    const Course* search(const string& courseNumber) const {
        string key = toUpper(courseNumber);
        Node* cur = root;
        while (cur != nullptr) {
            if (key == cur->data.number) return &cur->data;
            if (key < cur->data.number) cur = cur->left;
            else cur = cur->right;
        }
        return nullptr;
    }

    // Print all courses sorted by course number
    void printInOrder() const {
        inOrderRec(root);
    }

    bool empty() const { return root == nullptr; }

private:
    Node* root;

    static Node* insertRec(Node* node, const Course& c) {
        if (!node) return new Node(c);
        if (c.number < node->data.number) node->left = insertRec(node->left, c);
        else if (c.number > node->data.number) node->right = insertRec(node->right, c);
        else {
            node->data.title = c.title;
            node->data.prerequisites = c.prerequisites;
        }
        return node;
    }

    static void inOrderRec(Node* node) {
        if (!node) return;
        inOrderRec(node->left);
        cout << node->data.number << ", " << node->data.title << endl;
        inOrderRec(node->right);
    }

    static void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
};

/* -------------------------------------------------------------
   CSV parsing helper
   Format: number,title,prereq1,prereq2,...
------------------------------------------------------------- */

static vector<string> splitCsvLine(const string& line) {
    vector<string> tokens;
    string token;
    stringstream ss(line);
    while (getline(ss, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

/* -------------------------------------------------------------
   Load CSV and populate tree
------------------------------------------------------------- */

static bool loadCoursesFromCsv(const string& filename, CourseBST& tree) {
    ifstream in(filename);
    if (!in.is_open()) {
        cout << "Error: could not open file " << filename << endl;
        return false;
    }

    vector<Course> staging;
    unordered_map<string, size_t> indexByNumber;
    string line;
    size_t lineNo = 0;

    while (getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty()) continue;

        vector<string> parts = splitCsvLine(line);
        if (parts.size() < 2) {
            cout << "Warning: line " << lineNo << " missing fields and will be skipped" << endl;
            continue;
        }

        Course c;
        c.number = toUpper(parts[0]);
        c.title = parts[1];
        for (size_t i = 2; i < parts.size(); ++i) {
            if (!parts[i].empty()) c.prerequisites.push_back(toUpper(parts[i]));
        }

        indexByNumber[c.number] = staging.size();
        staging.push_back(move(c));
    }

    in.close();

    for (const Course& c : staging) {
        tree.insert(c);
    }

    cout << "Courses loaded: " << staging.size() << endl;
    return true;
}

/* -------------------------------------------------------------
   Print one course and its prerequisites
------------------------------------------------------------- */

static void printSingleCourse(const CourseBST& tree) {
    cout << "Enter course number: ";
    string query;
    getline(cin, query);
    query = trim(query);

    if (query.empty()) {
        cout << "No course number entered" << endl;
        return;
    }

    const Course* c = tree.search(query);
    if (!c) {
        cout << "Course not found" << endl;
        return;
    }

    cout << c->number << ", " << c->title << endl;
    if (c->prerequisites.empty()) {
        cout << "Prerequisites: None" << endl;
    }
    else {
        cout << "Prerequisites: ";
        for (size_t i = 0; i < c->prerequisites.size(); ++i) {
            cout << c->prerequisites[i];
            if (i + 1 < c->prerequisites.size()) cout << ", ";
        }
        cout << endl;
    }
}

/* -------------------------------------------------------------
   Menu handling
------------------------------------------------------------- */

static void printMenu() {
    cout << endl;
    cout << "Menu:" << endl;
    cout << "  1. Load Data Structure" << endl;
    cout << "  2. Print Course List" << endl;
    cout << "  3. Print Course" << endl;
    cout << "  9. Exit" << endl;
    cout << "Enter choice: ";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    CourseBST tree;
    bool running = true;

    while (running) {
        printMenu();
        string choiceLine;
        getline(cin, choiceLine);
        if (choiceLine.empty()) continue;

        int choice = 0;
        try {
            choice = stoi(choiceLine);
        }
        catch (...) {
            cout << choiceLine << " is not a valid option" << endl;
            continue;
        }

        switch (choice) {
        case 1: {
            cout << "Enter file name to load: ";
            string filename;
            getline(cin, filename);
            filename = trim(filename);
            if (filename.empty()) {
                cout << "No file name entered" << endl;
                break;
            }
            CourseBST newTree;
            if (loadCoursesFromCsv(filename, newTree)) {
                tree = std::move(newTree); // now safely transfers ownership
            }
            break;
        }

        case 2:
            if (tree.empty()) {
                cout << "Please load the data structure first" << endl;
            }
            else {
                tree.printInOrder();
            }
            break;

        case 3:
            if (tree.empty()) {
                cout << "Please load the data structure first" << endl;
            }
            else {
                printSingleCourse(tree);
            }
            break;

        case 9:
            cout << "Good bye" << endl;
            running = false;
            break;

        default:
            cout << choice << " is not a valid option" << endl;
            break;
        }
    }

    return 0;
}
