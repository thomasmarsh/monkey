#!/usr/bin/env python
import re
import sys

win_count = []

def add_win(i):
    while i+1 > len(win_count):
        win_count.append(0)
    win_count[i] += 1

challenge_marker = re.compile(' BEGIN CHALLENGE #(\\d+)')
player_marker    = re.compile(' - player (\\d+): (\\d+)')

class Parser:
    def __init__(self, fname):
        self.state = 0
        self.best = -1
        self.high = 0
        self.parse(fname)

    def parse_line(self, line):
        if self.state == 0:
            m = challenge_marker.search(line)
            if m and m.group(1) == '16':
                self.state = 1

        if self.state == 1:
            m = player_marker.search(line)
            if m:
                player = int(m.group(1))
                score = int(m.group(2))
                if score > self.high:
                    self.high = score
                    self.best = player

    def parse(self, fname):
        with open(fname, 'r') as f:
            for line in f.readlines():
                self.parse_line(line)
        if self.best != -1:
            add_win(self.best)

assert(len(sys.argv) > 1)
for file in sys.argv[1:]:
    #print file
    Parser(file)

print win_count
