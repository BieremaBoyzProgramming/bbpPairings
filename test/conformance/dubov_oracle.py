#!/usr/bin/env python3
"""Conformance bench for the Dubov engine (FIDE C.04.4.1, 2026 rules).

Generates random tournaments with bbpPairings (--dubov -g), then re-pairs every
round from the truncated history with bbpPairings and with external oracle
engines, and reports the rounds where the pairings differ.

Oracles are optional command lines; examples:
  --oracle chesspairing="/path/chesspairing --dubov {trf} -p {out}"
  --oracle py4swiss="/path/py4swiss -e dubov -t {trf} -p {out}"

Usage:
  dubov_oracle.py --bbp ./bbpPairings.exe --tournaments 50 [--oracle ...]

The "bbp" line checks that bbpPairings reproduces the pairings it generated
(the script exits with status 1 otherwise). The oracle lines count the rounds
where another engine agrees or differs; differences are expected wherever the
engines interpret the rules differently.
"""

import argparse
import os
import random
import subprocess
import sys
import tempfile
from collections import Counter

ROUND_WIDTH = 10
RESULTS_COLUMN = 89


def points_of(result, bye_points=10):
    return {
        '1': 10, '+': 10, 'W': 10, 'U': bye_points, '=': 5, 'D': 5, 'H': 5,
        '0': 0, '-': 0, 'L': 0, 'Z': 0, 'F': 10,
    }.get(result, 0)


def parse_rounds(line):
    rounds = []
    text = line[RESULTS_COLUMN:]
    for start in range(0, len(text), ROUND_WIDTH):
        chunk = text[start:start + ROUND_WIDTH]
        if len(chunk.strip()) == 0:
            continue
        chunk = chunk.ljust(ROUND_WIDTH)
        rounds.append((chunk[2:6].strip(), chunk[7], chunk[9]))
    return rounds


def is_unpaired(entry):
    opponent, _, result = entry
    return opponent in ('', '0000') and result != 'U'


def read_trf(path):
    with open(path, newline='') as stream:
        text = stream.read().replace('\r\n', '\n').replace('\r', '\n')
    return [line for line in text.split('\n') if line]


def truncate(lines, round_count, total_rounds, initial_color):
    """Return the TRF lines keeping only the first round_count rounds."""
    result = []
    for line in lines:
        if line.startswith('001'):
            all_rounds = parse_rounds(line)
            rounds = all_rounds[:round_count]
            # Keep the announced absences and requested byes of the round to
            # be paired: those players are not paired (GHR Article 3.3).
            if len(all_rounds) > round_count and is_unpaired(all_rounds[round_count]):
                rounds = rounds + [all_rounds[round_count]]
            score = sum(points_of(r[2]) for r in rounds)
            head = line[:RESULTS_COLUMN].ljust(RESULTS_COLUMN)
            head = head[:80] + ('%4.1f' % (score / 10)).rjust(4) + head[84:]
            body = ''.join(
                '  %4s %s %s' % (opponent.rjust(4), color, result)
                for opponent, color, result in rounds)
            result.append((head + body).rstrip())
        elif line.startswith(('012', 'XXA', 'XXP')):
            result.append(line)
    result.append('XXR %d' % total_rounds)
    result.append('XXC %s1' % initial_color)
    return result


def actual_pairing(lines, round_index):
    pairs = set()
    for line in lines:
        if not line.startswith('001'):
            continue
        player = int(line[4:8])
        rounds = parse_rounds(line)
        if round_index >= len(rounds):
            continue
        opponent, color, result = rounds[round_index]
        if result == 'U':
            pairs.add((player, 0))
        elif opponent not in ('', '0000') and color == 'w':
            pairs.add((player, int(opponent)))
    return pairs


def read_pairs(path):
    with open(path) as stream:
        values = stream.read().split()
    count = int(values[0])
    numbers = list(map(int, values[1:1 + 2 * count]))
    return {(numbers[2 * i], numbers[2 * i + 1]) for i in range(count)}


def add_implicit_bye(pairs, expected):
    """Some engines leave the pairing-allocated bye out of their output."""
    if any(black == 0 for _, black in pairs):
        return pairs
    paired = {player for pair in pairs for player in pair}
    everybody = {player for pair in expected for player in pair if player}
    unpaired = everybody - paired
    if len(unpaired) == 1:
        return pairs | {(unpaired.pop(), 0)}
    return pairs


def unordered(pairs):
    return {frozenset(pair) for pair in pairs}


def run(command):
    return subprocess.run(
        command, shell=True, stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE, timeout=600).returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bbp', required=True)
    parser.add_argument('--tournaments', type=int, default=20)
    parser.add_argument('--seed', type=int, default=2026)
    parser.add_argument('--oracle', action='append', default=[])
    parser.add_argument('--keep', help='directory where to keep differing cases')
    parser.add_argument('--workdir', help='directory for the temporary files')
    arguments = parser.parse_args()

    oracles = dict(entry.split('=', 1) for entry in arguments.oracle)
    rng = random.Random(arguments.seed)
    stats = {name: Counter() for name in ['bbp'] + list(oracles)}
    work = tempfile.mkdtemp(prefix='dubov-bench-', dir=arguments.workdir)

    for index in range(arguments.tournaments):
        players = rng.randint(6, 60)
        rounds = rng.randint(3, min(11, players - 1))
        config = os.path.join(work, 'config.txt')
        with open(config, 'w') as stream:
            stream.write('PlayersNumber=%d\nRoundsNumber=%d\n' % (players, rounds))
            stream.write('DrawPercentage=%d\n' % rng.randint(0, 50))
            # The rates are "one in N"; leaving them out disables the event.
            for key, choices in (
                    ('ForfeitRate', [None, None, 20, 50]),
                    ('RetiredRate', [None, None, 30, 100]),
                    ('HalfPointByeRate', [None, None, 30, 100])):
                value = rng.choice(choices)
                if value:
                    stream.write('%s=%d\n' % (key, value))
            stream.write('HighestRating=2700\nLowestRating=1200\n')
        trf = os.path.join(work, 't%03d.trf' % index)
        seed = rng.randint(1, 10 ** 9)
        if run('%s --dubov -g %s -o %s -s %d' % (arguments.bbp, config, trf, seed)):
            print('generation failed: players=%d rounds=%d seed=%d' % (players, rounds, seed))
            stats['bbp']['generation-failed'] += 1
            continue
        lines = read_trf(trf)
        first = [parse_rounds(line) for line in lines if line.startswith('001')]
        initial = 'white'
        for line in lines:
            if line.startswith('001') and int(line[4:8]) == 1:
                r = parse_rounds(line)
                if r and r[0][1] == 'b':
                    initial = 'black'
        played = max(len(r) for r in first)
        for round_index in range(played):
            truncated = os.path.join(work, 't%03d_r%02d.trf' % (index, round_index + 1))
            with open(truncated, 'w') as stream:
                stream.write('\n'.join(truncate(lines, round_index, rounds, initial)) + '\n')
            expected = actual_pairing(lines, round_index)
            engines = {'bbp': '%s --dubov {trf} -p {out}' % arguments.bbp}
            engines.update(oracles)
            for name, template in engines.items():
                out = truncated + '.' + name + '.out'
                if os.path.exists(out):
                    os.remove(out)
                code = run(template.format(trf=truncated, out=out))
                stats[name]['cases'] += 1
                if code or not os.path.exists(out):
                    stats[name]['error'] += 1
                    continue
                got = add_implicit_bye(read_pairs(out), expected)
                if got == expected:
                    stats[name]['identical'] += 1
                else:
                    same_pairs = unordered(got) == unordered(expected)
                    stats[name]['colours-differ' if same_pairs else 'pairs-differ'] += 1
                    if arguments.keep:
                        os.makedirs(arguments.keep, exist_ok=True)
                        subprocess.run(['cp', truncated, out, arguments.keep])
    for name, counter in stats.items():
        print('%-14s %s' % (name, dict(counter)))
    return 0 if stats['bbp']['cases'] == stats['bbp']['identical'] else 1


if __name__ == '__main__':
    sys.exit(main())
