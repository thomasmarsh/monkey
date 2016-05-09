#!/usr/bin/env python

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import re
import random
import math
import sys
import os

fname = sys.argv[1]

tableau20 = [
    (31, 119, 180),  (174, 199, 232), (255, 127, 14),  (255, 187, 120),    
    (44, 160, 44),   (152, 223, 138), (214, 39, 40),   (255, 152, 150),    
    (148, 103, 189), (197, 176, 213), (140, 86, 75),   (196, 156, 148),    
    (227, 119, 194), (247, 182, 210), (127, 127, 127), (199, 199, 199),    
    (188, 189, 34),  (219, 219, 141), (23, 190, 207),  (158, 218, 229)
]

for i in range(len(tableau20)):    
    r, g, b = tableau20[i]    
    tableau20[i] = (r / 255., g / 255., b / 255.)    

data = []
p = []
pr = [0,0]
d = []
dr = [0,0]
challenges = []
score = []
score_r = [0,0]
labels = []
active = []
handval = []
play_prog = None

state = 0
num = 0
with open(fname, 'r') as f:
    for line in f.readlines():
        if state == 0:
            #print '@@@@ INITIAL'
            assert(line.startswith('Players:'))
            state = 1
            #print line
            continue
        if state == 1:
            #print '@@@@ PARSE PLAYER'
            if line.startswith('- '):
                #print '@@@@  > PLAYER'
                m = re.search('- \\d+: (.*)\n', line)
                labels.append(m.group(1))
                p.append([])
                d.append([])
                active.append([True])
                score.append([])
                handval.append([])
                #print line
                continue
            else:
                #print '@@@@  > END PLAYERS'
                prog = ' '.join(map(lambda x: 'p%d=(\\d+)' % x, range(len(p))))
                num = len(p)
                play_prog = re.compile(prog)
                state = 2

        if state == 3:
            #print '@@@@ LOOK FOR SCORE'
            m = re.search('player (\\d+): (\\d+)', line)
            if m:
                player = int(m.group(1))
                value = int(m.group(2))
                score[player].append(value)
                if value > score_r[0]: score_r[0] = value
                if value < score_r[1]: score_r[1] = value
                #print line
                continue
            else:
                #print '@@@@ DONE: LOOK FOR SCORE'
                state = 2

        if state == 2:
            #print '@@@@ GENERAL'
            m = play_prog.search(line)
            if m:
                #print '@@@@ FOUND PLAY'
                for i in range(len(p)):
                    try:
                        prev = p[i][-1]
                    except: prev = 0

                    v = int(m.group(i+1))
                    if active[i]:
                        if v > pr[0]: pr[0] = v
                        if v < pr[1]: pr[1] = v
                        p[i].append(v)

                    if math.isnan(prev): d[i].append(0)
                    else:
                        delta = v-prev
                        if delta > dr[0]: dr[0] = delta
                        if delta < dr[1]: dr[1] = delta
                        d[i].append(delta)

            elif re.search(': player \\d+ \\(v=\\d+\\)', line):
                #print '@@@@ FOUND HAND'
                m = re.search(': player (\\d+) \\(v=(\\d+)\\)', line)
                player = int(m.group(1))
                value = int(m.group(2))
                handval[player].append(value)

            elif re.search('<player (\\d+):concede>', line):
                #print '@@@@ FOUND CONCEDE'
                m = re.search('<player (\\d+):concede>', line)
                active[int(m.group(1))] = False

            elif re.search(': score:', line):
                challenges.append([p, d])
                if len(p[0]) > 0:
                    n = len(p)
                    p = []
                    d = []
                    for i in range(n):
                        p.append([0])
                        d.append([0])
                        active[i] = True
                #print '@@@@ FOUND SCORE HEADING'
                state = 3
        #print line
        


def color(i):
    return tableau20[i]

def prepare_subplot(ax):
    # Remove the plot frame lines. They are unnecessary chartjunk.    
    ax.spines["top"].set_visible(False)    
   # ax.spines["bottom"].set_visible(False)    
    ax.spines["right"].set_visible(False)    
   # ax.spines["left"].set_visible(False)

    # Ensure that the axis ticks only show up on the bottom and left of the plot.    
    # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()
    ax.get_xaxis().set_visible(False)

def plot(i, data):
    plt.plot(data, lw=2.5, color=color(i), label=labels[i], clip_on=False)
    plt.plot(data, 'o', lw=0, markersize=3, color=color(i), clip_on=False)

def bar_plot(i, data):
    plt.bar(range(len(data)), data, color=color(i), align='center')

pdf = os.path.splitext(fname)[0] + '.pdf'
pp = PdfPages(pdf)

fig = plt.figure()
ax1 = plt.subplot2grid((1, 1), (0,0))
prepare_subplot(ax1)
for i in range(len(p)):
    plot(i, score[i])
for y in range(0, score_r[0], 50):
    plt.plot(range(len(score[0])), [y] * len(range(len(score[0]))), "--", lw=0.5, color="black", alpha=0.3)
plt.legend(loc='upper left')
pp.savefig(fig)


h = len(p)+1
w = 1

for j in range(len(score[0])):
    fig = plt.figure()
    ax2 = plt.subplot2grid((h, w), (0,0))
    ax2.set_title('Challenge %d' % (j+1))
    prepare_subplot(ax2)
    plt.ylim(pr[1]-1, pr[0]+1)

    for i in range(len(p)):
        c = challenges[j][0][i]
        plot(i, c)

        y = handval[i][j]
        plt.plot(range(len(c)), [y] * len(range(len(c))), "--", lw=1.5, color=color(i), alpha=0.8)

    for k in range(len(p)):
        ax3 = plt.subplot2grid((h, w), (k+1,0))
        prepare_subplot(ax3)
        plt.ylim(dr[1], dr[0])

        for i in range(len(p)):
            bar_plot(k, challenges[j][1][k])
            plt.subplots_adjust(hspace = .1)
    pp.savefig(fig)


pp.close()
os.system('open %s' % pdf)
