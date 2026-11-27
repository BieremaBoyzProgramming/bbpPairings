#!/usr/bin/env python3
"""Conformance bench for the Lim engine (FIDE C.04.4.3, 2026 rules).

Same method as dubov_oracle.py: random tournaments are generated with
bbpPairings (--lim -g), every round is re-paired from the truncated history
with bbpPairings and with external oracle engines, and the rounds where the
pairings differ are counted.

Oracles are optional command lines; examples:
  --oracle chesspairing="/path/chesspairing --lim {trf} -p {out}"
  --oracle reference="python3 lim_reference.py {trf} {out}"

Usage:
  lim_oracle.py --bbp ./bbpPairings.exe --tournaments 50 [--oracle ...]

The "bbp" line checks that bbpPairings reproduces the pairings it generated
(the script exits with status 1 otherwise). The --generator option generates
the tournaments with another system (for example dutch), in which case the
"bbp" line only counts how often the Lim engine agrees with that system.
"""

import argparse
import os
import random
import subprocess
import sys
import tempfile
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from dubov_oracle import (  # noqa: E402
    actual_pairing,
    add_implicit_bye,
    parse_rounds,
    read_pairs,
    read_trf,
    run,
    truncate,
    unordered,
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bbp', required=True)
    parser.add_argument('--tournaments', type=int, default=20)
    parser.add_argument('--seed', type=int, default=2026)
    parser.add_argument('--min-players', type=int, default=6)
    parser.add_argument('--max-players', type=int, default=60)
    parser.add_argument('--generator', default='lim')
    parser.add_argument('--oracle', action='append', default=[])
    parser.add_argument('--keep', help='directory where to keep differing cases')
    parser.add_argument('--workdir', help='directory for the temporary files')
    arguments = parser.parse_args()

    oracles = dict(entry.split('=', 1) for entry in arguments.oracle)
    rng = random.Random(arguments.seed)
    stats = {name: Counter() for name in ['bbp'] + list(oracles)}
    work = tempfile.mkdtemp(prefix='lim-bench-', dir=arguments.workdir)

    for index in range(arguments.tournaments):
        players = rng.randint(arguments.min_players, arguments.max_players)
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
        if run('%s --%s -g %s -o %s -s %d' % (
                arguments.bbp, arguments.generator, config, trf, seed)):
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
            engines = {'bbp': '%s --lim {trf} -p {out}' % arguments.bbp}
            engines.update(oracles)
            for name, template in engines.items():
                out = truncated + '.' + name + '.out'
                if os.path.exists(out):
                    os.remove(out)
                code = run(template.format(trf=truncated, out=out))
                stats[name]['cases'] += 1
                if code or not os.path.exists(out):
                    stats[name]['error'] += 1
                    if arguments.keep:
                        os.makedirs(arguments.keep, exist_ok=True)
                        subprocess.run(['cp', truncated, arguments.keep])
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
