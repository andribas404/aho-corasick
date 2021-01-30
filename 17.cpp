/*

Copyright 2019 Andrey Petukhov

Implementation of:

Alfred V. Aho and Margaret J. Corasick. 1975.
Efficient string matching: an aid to bibliographic search.
Commun. ACM 18, 6 (June 1975), 333-340.
DOI=http://dx.doi.org/10.1145/360825.360855

17. Шаблон с ?
Все языки	make2
Ограничение времени	3 секунды	80 секунд
Ограничение памяти	12Mb	400Mb
Ввод	стандартный ввод или input.txt
Вывод	стандартный вывод или output.txt

Шаблон поиска задан строкой длины m, в которой кроме обычных символов могут встречаться символы “?”.
Найти позиции всех вхождений шаблона в тексте длины n.
Каждое вхождение шаблона предполагает, что все обычные символы совпадают с соответствующими из текста,
а вместо символа “?” в тексте встречается произвольный символ.

Время работы - O(n + m + Z),
где Z - общее -число вхождений подстрок шаблона “между вопросиками” в исходном тексте.
m ≤ 5000, n ≤ 2000000.

Пример 1
Ввод	Вывод
ab??aba
ababacaba
2
*/

#include <cstdio>
#include <iostream>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::vector;
using std::string;

size_t ALPHABET_SIZE = 256;


struct Node {
    // """Trie Node."""
    Node() {}
    // # Таблица переходов из узла
    std::unordered_map<char, Node*> links;
    // # Является ли узел терминальным
    bool terminal = false;
    // # Ключевые слова, окончанием которых является данный узел
    std::unordered_set<short> output;
    // # ребро перехода если не подошла таблица переходов
    Node* link_fail = nullptr;
};

class Trie {
    // """Trie."""
 public:
    Trie() {
        // """Инициализация Trie."""
        root = new Node();
    }

    ~Trie() {
        vector<Node*> q;
        std::unordered_set<Node*> d;
        q.push_back(root);
        while (!q.empty()) {
            auto node = q.back();
            q.pop_back();
            for (auto it = node->links.cbegin(); it != node->links.cend(); ++it) {
                auto node_next = it->second;
                if (d.count(node_next)) {
                    continue;
                }
                q.push_back(node_next);
                d.insert(node_next);
            }
        }
        for (auto it = d.begin(); it != d.end(); it++) {
            delete *it;
        }
    }

    void add_word(const vector<char>& word, short ind) {
        // """Добавление ключевого слова."""
        // # начинаем от корня
        auto node = root;
        for (auto c : word) {
            // # если надо - создаем промежуточные узлы
            Node* next_node;
            try {
                next_node = node->links.at(c);
            } catch (const std::out_of_range& exception) {
                next_node = new Node();
                node->links[c] = next_node;
            }
            node = next_node;
        }
        // # последний узел - терминальный
        node->terminal = true;
        // # и выдает номер слова (множество)
        node->output.insert(ind);
    }
protected:
    Node* root;
};

class PatternMatchingMachine : public Trie {
    // """Pattern Matching Machine."""
 public:
    explicit PatternMatchingMachine(const vector<char>& pattern);
    ~PatternMatchingMachine();
    Node* goto_(Node* state, char c);
    Node* failure(Node* state);
    void build();
    void build_failure();
    void split_to_words();
    void process(char c);
    void match(int pos, Node* state);
 private:
    vector<char> pattern;
    int pattern_size;
    Node* state_;
    int counter;
    std::deque<short>* window;
    int window_start;
    vector<std::pair<vector<char>, short>> words;
};

PatternMatchingMachine::PatternMatchingMachine(const vector<char>& pattern) :
    Trie(),
    // # запоминаем шаблон
    pattern(pattern),
    // # его длину
    pattern_size(pattern.size()),
    // # начальное состояние - корень Trie
    state_(root),
    // # сколько букв пройдено (пробег)
    counter(0)
{
    // """Инициализация машины совмещения шаблонов."""

    // # окно соответствия. сдвигается с потоком, window[i] = множество слов,
    // # которые имеют окончание в этой позиции
    window = new std::deque<short>(pattern_size, 0);
    // # начальная позиция окна
    window_start = 0;
    // # строим машину
    build();
}

PatternMatchingMachine::~PatternMatchingMachine() {
    delete window;
}

Node* PatternMatchingMachine::goto_(Node* state, char c) {
    // """
    // Goto function.

    // Функция переходов. Из корня всегда есть путь.
    // """
    try {
        auto node = state->links.at(c);
        return node;
    } catch (const std::out_of_range& exception) {
    }
    if (state == root) {
        return root;
    }
    return nullptr;
}

Node* PatternMatchingMachine::failure(Node* state) {
    // """Failure function."""
    return state->link_fail;
}

void PatternMatchingMachine::build() {
    // """Build Pattern Matching Machine."""
    split_to_words();
    // # print(self.words)
    // # build goto (Алгоритм №2)
    for (auto ind = 0; ind < words.size(); ind++) {
        auto word = words[ind].first;
        auto pos = words[ind].second;
        add_word(word, pos);
    }
    // # build failure
    build_failure();
}

void PatternMatchingMachine::build_failure() {
    // """
    // Add failure transitions.

    // Алгоритм №3
    // """
    std::deque<Node*> q;
    // # DONE: iterate map
    for (auto it = root->links.begin(); it != root->links.end(); it++) {
        auto node = it->second;
        node->link_fail = root;
        q.push_back(node);
    }

    while (!q.empty()) {
        auto node = q.front();
        q.pop_front();
        if (node->links.empty()) {
            continue;
        }
        // # DONE: iterate map
        for (auto it = node->links.begin(); it != node->links.end(); it++) {
            auto c = it->first;
            auto node_next = it->second;
            auto state = node->link_fail;
            while (goto_(state, c) == nullptr) {
                state = state->link_fail;
            }
            node_next->link_fail = goto_(state, c);
            // # DONE: set() | set()
            node_next->output.insert(
                node_next->link_fail->output.begin(),
                node_next->link_fail->output.end()
            );
            if (!node_next->output.empty()) {
                node_next->terminal = true;
            }
            q.push_back(node_next);
        }
    }
}

void PatternMatchingMachine::split_to_words() {
    // """
    // Разбиение слова на список.

    // [(<слово>, <позиция где заканчивается>),...]
    // """
    vector<char> word;
    int pos = -1;
    for (auto c : pattern) {
        if (c == '?') {
            if (!word.empty()) {
                words.push_back(std::make_pair(word, pos));
                word.clear();
            }
        } else {
            word.push_back(c);
        }
        pos++;
    }
    if (!word.empty()) {
        words.push_back(std::make_pair(word, pos));
    }
}

void PatternMatchingMachine::process(char c) {
    // """Process one character."""
    // # увеличиваем пробег
    counter++;

    // # крайний случай
    if (words.empty()) {
        // # DONE: all "?" case
        if (counter < pattern_size) {
            return;
        }
        if (pattern_size) {
            printf("%d ", counter - pattern_size);
        } else {
            printf("%d ", counter - 1);
        }
        return;
    }

    vector<Node*> path;
    // # ищем первый узел куда можно перейти (Алгоритм №1)
    while (goto_(state_, c) == nullptr) {
        path.push_back(state_);
        state_ = failure(state_);
    }
    state_ = goto_(state_, c);
    for (auto node : path) {
        node->links[c] = state_;
    }

    // # сопоставляем окончание найденных слов с окном
    match(counter, state_);

    if (counter < pattern_size) {
        return;
    }

    if (window->at(0) == words.size()) {
        printf("%d ", counter - pattern_size);
    }

    // # сдвигаем окно
    window->pop_front();
    window->push_back(0);
    window_start++;
}

void PatternMatchingMachine::match(int pos, Node* state) {
    // # print(f"{pos}: {words}")
    // # DONE: iterate set()
    if (!state->terminal) {
        return;
    }
    for (auto it = state->output.begin(); it != state->output.end(); it++) {
        auto word_pos = *it;

        auto pos_relative = pos - word_pos - 1 - window_start;
        if (pos_relative < 0) {
            continue;
        }
        window->at(pos_relative)++;
    }
}

int main() {
    vector<char> pattern;
    for (int c = getchar(); c > 32; c = getchar()) {
        pattern.push_back(c);
    }

    auto machine = new PatternMatchingMachine(pattern);

    for (int c = getchar(); c != EOF;) {
        if (c > 32) {
            machine->process(c);
        }
        c = getchar();
    }

    delete machine;
    return 0;
}
