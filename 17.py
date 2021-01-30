#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
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
"""

import sys
from collections import deque


class Node:
    """Trie Node."""

    def __init__(self):
        """Инициализация узла."""
        # Таблица переходов из узла
        self.links = dict()
        # Является ли узел терминальным
        self.terminal = False
        # Ключевые слова, окончанием которых является данный узел
        self.output = set()
        # ребро перехода если не подошла таблица переходов
        self.link_fail = None


class Trie:
    """Trie."""

    def __init__(self):
        """Инициализация Trie."""
        self.root = Node()

    def add_word(self, word, ind):
        """Добавление ключевого слова."""
        # начинаем от корня
        node = self.root
        for c in word:
            # если надо - создаем промежуточные узлы
            next_node = node.links.setdefault(c, Node())
            node = next_node
        # последний узел - терминальный
        node.terminal = True
        # и выдает номер слова (множество)
        node.output.add(ind)


class PatternMatchingMachine(Trie):
    """Pattern Matching Machine."""

    def __init__(self, pattern):
        """Инициализация машины совмещения шаблонов."""
        super().__init__()
        # запоминаем шаблон
        self.pattern = pattern
        # его длину
        self.pattern_size = len(pattern)
        # начальное состояние - корень Trie
        self.state = self.root
        # сколько букв пройдено (пробег)
        self.counter = 0
        # окно соответствия. сдвигается с потоком, window[i] = множество слов,
        # которые имеют окончание в этой позиции
        self.window = deque([set() for _ in range(self.pattern_size)])
        # начальная позиция окна
        self.window_start = 0
        # строим машину
        self.build()

    def goto(self, state, c):
        """
        Goto function.

        Функция переходов. Из корня всегда есть путь.
        """
        if c in state.links:
            return state.links[c]
        if state == self.root:
            return self.root
        return None

    def failure(self, state):
        """Failure function."""
        return self.state.link_fail

    def output(self, state):
        """Output function."""
        if not state.terminal:
            return None
        return state.output

    def build(self):
        """Build Pattern Matching Machine."""
        self.words = self.split_to_words()
        self.precompute_deltas()
        # print(self.words)
        # build goto (Алгоритм №2)
        for ind, (word, pos) in enumerate(self.words):
            self.add_word(word, ind)
        # build failure
        self.build_failure()

    def precompute_deltas(self):
        self.deltas = []
        self.deltas.append(0)
        for i in range(1, len(self.words)):
            self.deltas.append(self.words[i][1] - self.words[i - 1][1])

    def build_failure(self):
        """
        Add failure transitions.

        Алгоритм №3
        """
        q = deque()
        for node in self.root.links.values():
            node.link_fail = self.root
            q.append(node)
        depth_current = 1
        while q:
            node = q.popleft()
            if not node.links:
                continue
            for c, node_next in node.links.items():
                state = node.link_fail
                while self.goto(state, c) is None:
                    state = state.link_fail
                node_next.link_fail = self.goto(state, c)
                node_next.output |= node_next.link_fail.output
                if (node_next.output):
                    node_next.terminal = True
                q.append(node_next)

    def split_to_words(self):
        """
        Разбиение слова на список.

        [(<слово>, <позиция где заканчивается>),...]
        """
        words = []
        word = []
        pos = -1
        for c in self.pattern:
            if c == "?":
                if word:
                    words.append((word, pos))
                    word = []
            else:
                word.append(c)
            pos += 1
        if word:
            words.append((word, pos))
        return words

    def process(self, c):
        """Process one character."""
        # увеличиваем пробег
        self.counter += 1

        # крайний случай
        if not self.words:
            if self.counter < self.pattern_size:
                return
            # DONE: all "?" case
            sys.stdout.write(f"{self.counter - self.pattern_size} ")
            return

        state_old = self.state
        # ищем первый узел куда можно перейти (Алгоритм №1)
        while self.goto(self.state, c) == None:
            self.state = self.failure(self.state)
        self.state = self.goto(self.state, c)
        state_old.links[c] = self.state

        # сопоставляем окончание найденных слов с окном
        if self.output(self.state) is not None:
            self.match(self.counter, self.output(self.state))

        # print(f"{self.counter:3d}: {self.output(self.state)}")

        if self.counter < self.pattern_size:
            return

        # индекс и место, где должно было закончиться последнее слово
        last_word_ind = len(self.words) - 1
        last_word_pos = self.words[-1][1]
        if last_word_ind in self.window[last_word_pos]:
            sys.stdout.write(f"{self.counter - self.pattern_size} ")

        # сдвигаем окно
        self.window.popleft()
        self.window.append(set())
        self.window_start += 1

    def match(self, pos, words):
        # print(f"{pos}: {words}")
        for word_ind in words:
            pos_relative = pos - self.window_start - 1
            if word_ind == 0:
                # слово - первое, добавляем всегда
                self.window[pos_relative].add(word_ind)
            else:
                # место, где ожидается предыдущее слово
                prev_word_pos = pos_relative - self.deltas[word_ind]
                if prev_word_pos < 0:
                    continue
                prev_word_ind = word_ind - 1
                if prev_word_ind in self.window[prev_word_pos]:
                    self.window[pos_relative].add(word_ind)


if __name__ == "__main__":
    pattern = input().strip()
    # print(pattern)
    machine = PatternMatchingMachine(pattern)
    while True:
        c = sys.stdin.read(1)
        if c == "":
            break
        if ord(c) < 33:
            continue
        machine.process(c)
