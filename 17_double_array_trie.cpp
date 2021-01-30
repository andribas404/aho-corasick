/*

Copyright 2019 Andrey Petukhov

Implementation of:

Alfred V. Aho and Margaret J. Corasick. 1975.
Efficient string matching: an aid to bibliographic search.
Commun. ACM 18, 6 (June 1975), 333-340.
DOI=http://dx.doi.org/10.1145/360825.360855

J. I. Aoe. 1989.
An efficient implemenatation of string pattern matching machines
for a finite number of keywords.
SIGIR Forum 23, 3-4 (April 1989), 22-33.
DOI=http://dx.doi.org/10.1145/74697.74699

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
#include <deque>
#include <unordered_set>
#include <vector>

using std::vector;

size_t ALPHABET_SIZE = 256;
size_t ALPHABET_SIZE_ALIGNED = ALPHABET_SIZE;
size_t MAX_SIZE = 1 << 13;

struct Node {
    // """Trie Node."""
    explicit Node(short node_id) : node_id(node_id), links(ALPHABET_SIZE, 0) {}
    // get link
    inline short get_link(unsigned char c) { return links[c]; }
    // set link
    inline void set_link(unsigned char c, short node_id) { links[c] = node_id; }
    // set link
    inline void set_link(unsigned char c, Node* node) { links[c] = node->node_id; }

    // node id
    short node_id;
    // # Таблица переходов из узла (node ids)
    vector<short> links;
    // # Является ли узел терминальным
    bool terminal = false;
    // # Ключевые слова, окончанием которых является данный узел (позиция слова)
    std::unordered_set<short> output;
    // # ребро перехода если не подошла таблица переходов
    Node* link_fail = nullptr;
};

class Trie {
    // """Trie."""
 public:
    Trie() {
        // """Инициализация Trie."""
        nodes.push_back(nullptr);
        root = add_node();
    }

    ~Trie() {
        for (auto node : nodes) {
            delete node;
        }
    }

    Node* add_node() {
        auto node = new Node(nodes.size());
        nodes.push_back(node);
        return node;
    }

    void add_word(const vector<unsigned char>& word, short ind) {
        // """Добавление ключевого слова."""
        // # начинаем от корня
        auto node = root;
        for (auto c : word) {
            // # если надо - создаем промежуточные узлы
            auto node_id = node->get_link(c);
            Node* next_node = nodes[node_id];
            if (next_node == nullptr) {
                next_node = add_node();
                node->set_link(c, next_node);
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
    vector<Node*> nodes;
};

class PatternMatchingMachine : public Trie {
    // """Pattern Matching Machine."""
 public:
    explicit PatternMatchingMachine(const vector<unsigned char>& pattern);
    ~PatternMatchingMachine();
    inline Node* goto_(Node* state, unsigned char c);
    Node* failure(Node* state);
    void build();
    void build_failure();
    void do_bfs();
    void split_to_words();
    void process(unsigned char c);
    void match(int pos, Node* state);
    int words_size();
    int nodes_size();
    void make_flat(int** transitions, short*** output);

 private:
    vector<unsigned char> pattern;
    int pattern_size;
    Node* state_;
    int counter;
    std::deque<short>* window;
    int window_start;
    vector<std::pair<vector<unsigned char>, short>> words;
};

PatternMatchingMachine::PatternMatchingMachine(const vector<unsigned char>& pattern) :
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

inline Node* PatternMatchingMachine::goto_(Node* state, unsigned char c) {
    // """
    // Goto function.

    // Функция переходов. Из корня всегда есть путь.
    // """
    return nodes[state->get_link(c)];
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
    for (auto it = words.cbegin(); it != words.end(); it++) {
        auto ind = it - words.cbegin();
        auto word = words[ind].first;
        auto pos = words[ind].second;
        add_word(word, pos);
    }
    // # build failure
    build_failure();
    do_bfs();
}

void PatternMatchingMachine::build_failure() {
    // """
    // Add failure transitions.

    // Алгоритм №3
    // """
    std::deque<Node*> q;
    // # DONE: iterate map
    for (auto it = root->links.cbegin(); it != root->links.cend(); it++) {
        auto c = (unsigned char)(it - root->links.cbegin());
        auto node_id = *it;
        auto node = nodes[node_id];
        if (node == nullptr) {
            root->set_link(c, root);
            continue;
        }
        node->link_fail = root;
        q.push_back(node);
    }

    while (!q.empty()) {
        auto node = q.front();
        q.pop_front();
        // # DONE: iterate map
        for (auto it = node->links.cbegin(); it != node->links.cend(); it++) {
            auto c = (unsigned char)(it - node->links.cbegin());
            auto node_id = *it;
            auto node_next = nodes[node_id];
            if (node_next == nullptr) {
                continue;
            }
            auto state = node->link_fail;
            while (goto_(state, c) == nullptr) {
                state = state->link_fail;
            }
            node_next->link_fail = goto_(state, c);
            // # DONE: set() | set()
            node_next->output.insert(
                node_next->link_fail->output.cbegin(),
                node_next->link_fail->output.cend()
            );
            if (!node_next->output.empty()) {
                node_next->terminal = true;
            }
            q.push_back(node_next);
        }
    }
}

void PatternMatchingMachine::do_bfs() {
    // Algorithm # 4
    std::deque<Node*> q;

    for (auto it = root->links.cbegin(); it != root->links.cend(); it++) {
        auto node_id = *it;
        auto node = nodes[node_id];
        if (node != root) {
            q.push_back(node);
        }
    }

    while (!q.empty()) {
        auto node = q.front();
        q.pop_front();
        for (auto it = node->links.cbegin(); it != node->links.cend(); it++) {
            auto c = (unsigned char)(it - node->links.cbegin());
            auto node_id = *it;
            auto node_next = nodes[node_id];
            if (node_next == nullptr) {
                Node* state = failure(node);
                state = goto_(state, c);
                node->set_link(c, state);
                continue;
            } else {
                q.push_back(node_next);
            }
        }
    }
}

void PatternMatchingMachine::split_to_words() {
    // """
    // Разбиение слова на список.

    // [(<слово>, <позиция где заканчивается>),...]
    // """
    vector<unsigned char> word;
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

void PatternMatchingMachine::process(unsigned char c) {
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

    state_ = goto_(state_, c);
    // # сопоставляем окончание найденных слов с окном
    match(counter, state_);

    if (counter < pattern_size) {
        return;
    }

    if (window->at(0) == (short)words.size()) {
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
    for (auto it = state->output.cbegin(); it != state->output.cend(); it++) {
        auto word_pos = *it;

        auto pos_relative = pos - word_pos - 1 - window_start;
        if (pos_relative < 0) {
            continue;
        }
        window->at(pos_relative)++;
    }
}

int PatternMatchingMachine::words_size() {
    return words.size();
}

int PatternMatchingMachine::nodes_size() {
    return nodes.size() - 1;
}

void PatternMatchingMachine::make_flat(int** transitions, short*** output) {
    *transitions = new int [ALPHABET_SIZE_ALIGNED * (nodes.size() - 1)]();
    *output = new short* [nodes.size() - 1];
    for (size_t i = 1; i < nodes.size(); i++) {
        for (size_t j = 0; j < ALPHABET_SIZE; j++) {
            auto pos = (i - 1) * ALPHABET_SIZE_ALIGNED + j;
            auto next_pos = (nodes[i]->get_link(j) - 1) * ALPHABET_SIZE_ALIGNED;
            (*transitions)[pos] = next_pos;
        }
        // 0 - окончание
        (*output)[i - 1] = new short [nodes[i]->output.size() + 1]();
        size_t j = 0;
        for (auto it = nodes[i]->output.cbegin(); it != nodes[i]->output.cend(); it++, j++) {
            (*output)[i - 1][j] = *it + 1;
        }
        nodes[i]->output.clear();
    }
}

int main() {
    vector<unsigned char> pattern;
    int c = getchar();
    while (c <= 32 && c != EOF) {
        c = getchar();
    }
    while (c > 32 && c != EOF) {
        pattern.push_back(c);
        c = getchar();
    }

    int pattern_size = pattern.size();
    auto machine = new PatternMatchingMachine(pattern);

    short* window = new short[MAX_SIZE]();
    int* transitions;
    short** output;

    machine->make_flat(&transitions, &output);
    int words_size = machine->words_size();
    int nodes_size = machine->nodes_size();
    int pos = 0;
    delete machine;

    int state = 0;

    for (;c != EOF; c = getchar()) {
        if (c <= 32) {
            continue;
        }
        state = transitions[state + c];
        pos++;
        auto out = output[state >> 8];
        for (int j = 0; out[j] > 0; j++) {
            int pos_next = pos - out[j];
            if (pos_next >= 0) {
                window[pos_next & (MAX_SIZE - 1)]++;
            }
        }
        int prev_pos = pos - pattern_size;
        if (prev_pos >= 0) {
            int prev_pos_masked =prev_pos & (MAX_SIZE - 1);
            if (window[prev_pos_masked] == words_size) {
                printf("%d ", prev_pos);
            }
            window[prev_pos_masked] = 0;
        }
    }

    delete [] transitions;
    for (int i = 0; i < nodes_size; i++) {
        delete [] output[i];
    }
    delete [] output;
    delete [] window;

    return 0;
}
