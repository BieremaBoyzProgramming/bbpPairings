#!/usr/bin/env python3

import random
import subprocess
import os
import math
import time
import argparse

sysrand = random.SystemRandom()

parser = argparse.ArgumentParser(description='Simulate a tournament.')
parser.add_argument('players', metavar='N', type=int,
                    help='the number of players')
parser.add_argument('rounds', metavar='R', type=int,
                    help='the number of rounds')
parser.add_argument('--algorithm', '-a', default='fast', help='pairing algorithm ("dutch", "burstein", or "fast"; default is "fast")')
parser.add_argument('--keep-output', '-k', action='store_true', help='keep .trfx output')
parser.add_argument('--compare', '-c', help='compare with another pairing algorithm ("dutch", "burstein", or "fast")')

args = parser.parse_args()

alg = args.algorithm
total_round_count = args.rounds
num_players = args.players
keep_output = args.keep_output
compare_with = args.compare

class Pairing:
    def __init__(self, score, color, opponent_num):
        self.score = score
        self.color = color
        self.opponent_num = opponent_num

class Player:
    def __init__(self, num):
        self.num = num
        self.rating = random.randint(1000, 2500)
        self.score = 0.0
        self.pairings = []

players = [Player(i + 1) for i in range(num_players)]
players.sort(key=lambda x: -x.rating)
for i in range(num_players):
    players[i].num = i + 1
player_map = {p.num: p for p in players}

def pair_round(round_num, algorithm):
    input_file = open(f'round{round_num}.trfx', 'w+')
    output_file_name = f"{input_file.name}.{algorithm}.txt"
    try:
        # Write to input file
        input_file.write('XXW 7\n')
        input_file.write('XXR %d\n' % total_round_count)
        input_file.write('XXC black1\n')
        for n, player in enumerate(players, 1):
            line_parts = ['001 {0: >7}  {1:74.1f}     '.format(n, player.score)]
            for pairing in player.pairings:
                opponent_num = pairing.opponent_num or '0000000'
                color = pairing.color
                score = 'U' if not pairing.opponent_num else '1' if pairing.score == 1 else '0' if pairing.score == 0 else '=' if pairing.score == 0.5 else ' '
                if score == ' ':
                    color = '-'
                line_parts.append('{0: >9} {1} {2}'.format(opponent_num, color, score))
            line_parts.append('\n')
            input_file.write(''.join(line_parts))
        input_file.flush()

        start = time.time()
        result = _call_proc(input_file.name, output_file_name, algorithm)
        end = time.time()
        print(end - start)
        if not result:
            return None

        pairs = _read_output(output_file_name)
        return pairs
    finally:
        input_file.close()
        if not keep_output:
            try:
                os.remove(input_file.name)
            except OSError:
                pass
            try:
                os.remove(output_file_name)
            except OSError:
                pass

def _call_proc(input_file_name, output_file_name, algorithm):
    folder = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..')
    win_exe_path = os.path.join(folder, 'bbpPairings.exe')
    other_exe_path = os.path.join(folder, 'bbpPairings')
    path = win_exe_path if os.path.exists(win_exe_path) else other_exe_path
    print(f'{path} --{algorithm} {input_file_name} -p {output_file_name}')
    proc = subprocess.Popen(f'./bbpPairings.exe --{algorithm} {input_file_name} -p {output_file_name}',
        shell=True, stdout=subprocess.PIPE)
    stdout = proc.communicate()[0]
    if proc.returncode != 0:
        print('bbpPairings return code: %s. Output: %s' % (proc.returncode, stdout))
        return False
    return True

def _read_output(output_file_name):
    with open(output_file_name) as output_file:
        pair_count = int(output_file.readline())
        pairs = []
        for _ in range(pair_count):
            w, b = output_file.readline().split(' ')
            if int(b) == 0:
                pairs.append((int(w), None))
            else:
                pairs.append((int(w), int(b)))
        return pairs


chances_by_rating_delta = [
    (0.40, 0.15, 0.45),
    (0.25, 0.10, 0.65),
    (0.20, 0.10, 0.70),
    (0.15, 0.10, 0.75),
    (0.10, 0.10, 0.80),
    (0.05, 0.00, 0.95),
]
chances_by_rating_delta_reversed = [tuple(reversed(chances)) for chances in chances_by_rating_delta]

def result_chances(rating_delta):
    rating_delta_index = int(min(math.floor(abs(rating_delta) / 100.0), 5))
    if rating_delta < 0:
        return chances_by_rating_delta_reversed[rating_delta_index]
    else:
        return chances_by_rating_delta[rating_delta_index]

def _simulate_results(pairs):
    for white, black in pairs:
        if black is None:
            player_map[white].score += 1
            player_map[white].pairings.append(Pairing(1, '-', None))
        else:
            chances = result_chances(player_map[white].rating - player_map[black].rating)
            r = sysrand.random()
            if r < chances[0]:
                player_map[white].score += 0.0
                player_map[white].pairings.append(Pairing(0.0, 'w', black))
                player_map[black].score += 1.0
                player_map[black].pairings.append(Pairing(1.0, 'b', white))
            elif r < chances[0] + chances[1]:
                player_map[white].score += 0.5
                player_map[white].pairings.append(Pairing(0.5, 'w', black))
                player_map[black].score += 0.5
                player_map[black].pairings.append(Pairing(0.5, 'b', white))
            else:
                player_map[white].score += 1.0
                player_map[white].pairings.append(Pairing(1.0, 'w', black))
                player_map[black].score += 0.0
                player_map[black].pairings.append(Pairing(0.0, 'b', white))

for round_index in range(total_round_count):
    pairs = pair_round(round_index + 1, alg)
    if compare_with:
        other_pairs = pair_round(round_index + 1, compare_with)
        if pairs and other_pairs:
            similarity = len(set(pairs).intersection(set(other_pairs))) / len(pairs)
            print(f'Similarity: {similarity}')
    if not pairs:
        break
    _simulate_results(pairs)

