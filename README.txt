BBP Pairings, a Swiss-system chess tournament engine
created by Bierema Boyz Programming
See LICENSE.txt for copyright and licensing information.

The most recent official version of this program can be downloaded at
<https://github.com/BieremaBoyzProgramming/bbpPairings/releases>.
The version at <https://github.com/BieremaBoyzProgramming/bbpPairings> may
include changes that have not yet been included in a release.


BBP Pairings is an engine for pairing players in a Swiss-system chess
tournament. It attempts to implement rules specified in the FIDE handbook. It is
not a full tournament manager, just an engine for computing the pairings.

The program currently implements the 2025 rules for the Dutch system (the
effective date for the rules was delayed to 2026). It also includes a flawed
implementation of a previous version of the Burstein system. The implementation
of the Burstein system has not been endorsed by FIDE. Finally, it implements the
Dubov system as specified in C.04.4.1 of the FIDE handbook and the Lim system
as specified in C.04.4.3 (both with the rules applied from 1 February 2026).
These implementations have not been endorsed by FIDE either; see the sections
"Dubov system" and "Lim system" below for the interpretations they make.

The program's interface is designed to be very similar to that described in the
advanced user manual for JaVaFo 1.4 (with permission to do so from the author,
Roberto Ricca). The remainder of this document lists the points of divergence.
For more informaton about JaVaFo, please visit <www.rrweb.org/javafo>. This
documentation assumes full understanding of the JaVaFo AUM.

Hereafter, we will use the name "JaVaFo" to refer to JaVaFo 1.4.

Tournament files
-------
BBP Pairings is designed to support the TRF-2026 file format, as specified by
FIDE's Technical Commission. Certain features like free points and adjustable
point values for adjourned games are not currently supported.

Backwards compatability is provided for the previous file format, TRF(bx), an
extension of the TRF(x) format defined for JaVaFo. The rest of this section
provides more info on how TRF(bx) extends TRF(x) and inferences performed while
reading tournament files.

The first extension may be viewed by some as a deviation from TRF(x). In JaVaFo,
if no acceleration values are specified using XXA codes in the TRF(x), then the
default is that there is no acceleration. This is appropriate for the Dutch
system, and BBP Pairings matches this behavior. However, the rules for the
Burstein system seem to define an acceleration system to be used by default for
the Burstein system. Thus, if there are no XXA codes in a TRF(xb) to be paired
using the Burstein system, BBP Pairings defaults to using the acceleration
system defined in the Burstein system rules. To override this, simply include at
least one XXA line, perhaps with an acceleration of 0.0, since if there are any
XXA lines, BBP Pairings will not apply the default acceleration system.

JaVaFo attempts to infer the point system used for the tournament (the number of
points for wins and for draws) from the players' scores listed in the TRF. This
is easy to do if not all players have exactly the same ratio of wins to draws,
and it is very difficult to construct a tournament paired using the Dutch rules
in which knowing the players' current scores is insufficient to compute the
correct pairings (though contrived examples do exist).

The Burstein system differs in that the point system plays an integral part in
ordering players in scoregroups. As a simple example, consider a tournament in
which there are no draws, but some games are unplayed. Since players' total
scores are adjusted by counting unplayed games as draws when calculating their
opponents' tiebreak scores, the program needs to know the number of points given
for draws and cannot infer it.

For this reason, we must introduce another extension to the TRF(bx) format. The
extension uses codes BBW, BBD, BBL, BBZ, BBF, and BBU for specifying the number
of points awarded for a win, draw, played loss, zero-point bye, forfeit loss, or
pairing-allocated bye, respectively. The format is
BBW pp.p
BBD pp.p
...
Thus, for a tournament with 3 points for wins and 1 point for draws, the TRF(bx)
should contain the lines
BBW  3.0
BBD  1.0
If BBU is not specified, it defaults to the score for a win. All other
parameters default individually to particular values (1.0, 0.5, or 0.0) if not
specified.

If the tournament uses the standard point system, these lines are not necessary.

Because this requirement causes an incompatibility with the TRF(x)
representation of non-standard point system tournaments that could be overlooked
when exchanging files, BBP Pairings checks that all players' scores are computed
correctly from the tournament results. If this is not the case, it issues an
error message and refuses to proceed.

The engine for the Burstein system expects the point value for pairing-allocated
byes to equal one of the other point value parameters when computing virtual
opponent scores for the Buchholz and Sonneborn-Berger scores. If this is not the
case, it approximates the pairing-allocated bye point value as a win, loss, or
draw (rounding down), only for the purposes of choosing the virtual opponent
score for the round. In an unplayed game counted as a win, the forfeit loss
score is currently used as the the virtual opponent score, not the played loss.
These behaviors may change in future versions if criticized.


BBP Pairings does not support random selection of the initial piece color. The
initial piece color must be specified when generating the pairings for the first
round of the tournament (or more precisely, any round of the tournament in which
no player has previously been assigned a color).

Let Round F refer to the first round where a color assignment is listed in the
TRF(xb) for a certain tournament. If the initial piece color of the tournament
is not specified, it is assumed to be the Round-F color of the highest player
who participated in the pairing of Round F or an earlier round (where "highest
player" is defined by pairing IDs or positional IDs, depending on whether the
"rank" configuration option is used). If this highest player does not have a
color listed for Round F, we use the Round-F color of the second highest player
who participated in the pairing of Round F or earlier, but reverse the color,
and so on.

Note that if pairing numbers are reordered after pairing the first round, the
inferred initial color might not be correct, and the correct initial color
should be specified for every round.


BBP Pairings outputs files using the codes introduced in the 2026 version of the
TRF, but it can also read files produced using the codes specified in the JaVaFo
AUM.


Random Tournament Generator
---------------------------
The point value parameters not supported in JaVaFo 1.4 can be set using the keys
PointsForLoss, PointsForForfeitLoss (applies to zero-point byes as well), and
PointsForPAB. All of these have default values of 0.0, except the last, which by
default is set equal to PointsForWin.

BBP Pairings allows the user to specify a random seed on the command line when
generating a random tournament. This can simplify data exchange when testing
and helps simplify reproducibility for bug reports.

The first line of the output file includes the seed used to generate the
tournament. The line is written before the tournament is generated in case the
program fails to generate the tournament for whatever reason, such as a program
crash.

Checklist
---------
The checklist format of BBP Pairings is similar to that used by JaVaFo. However,
since the Burstein system does not take into account previous floaters, those
columns have been removed. In their place are new columns for the Sonneborn-
Berger scores, Buchholz scores, and Median scores.

Note that the Buchholz scores and Median scores are each presented as two
numbers. The "Buchholz tiebreak" refers to the score used to compare two players
originating in the same scoregroup (assuming their SB scores are the same); it
is the sum of the player's opponents' scores, with adjustments made for unplayed
games. The "Buchholz score" is the product of the player's total score and her
"Buchholz tiebreak". This is the value used to compare two players originating
in different scoregroups (again, assuming their SB scores are the same). The
"Median tiebreak" and "Median score" are defined analogously.

For the Dutch system, the column C2 indicates whether a given player is eligible
for the pairing-allocated bye (Y if yes, N if no). The C14 and C16 columns
indicate the floating direction of the player on the previous and
next-to-previous rounds, respectively (U for upfloat, D for downfloat).

For the Dubov system, the column C2 has the same meaning, ARO is the average
rating of the opponents (Article 1.7), Up is the number of times the player has
been upfloated (followed by * for a maximum upfloater, Article 1.8), and UpP
indicates whether the player upfloated in the previous round.

For the Lim system, the column C2 has the same meaning, Due is the colour the
player is due (Articles 5.1, 5.5 and 5.6), Must the colour the player must
receive (Articles 5.1.2 and 5.3), and FlP indicates whether the player floated
in the previous round (Article 3.10).

BBP Pairings has the additional option to generate checklist files while
generating or checking tournaments, not just when pairing individual rounds. In
this case, the tables for all of the rounds are put into the same file, with
round number indications appearing between them.

Command line arguments
----------------------
BBP Pairings always performs pairings using a weighted matching algorithm, so it
does not use the -w and -q options of JaVaFo.

Since BBP Pairings supports more than one Swiss system, an argument specifying
which Swiss system to use for pairing is required.

The acceptable syntax forms for running BBP Pairings are:
bbpPairings.exe [-r]
bbpPairings.exe [-r] (--burstein | --dubov | --dutch | --lim) input-file -c [-l [check-list-file]]
bbpPairings.exe [-r] (--burstein | --dubov | --dutch | --lim) input-file -p [output-file] [-l [check-list-file]]
bbpPairings.exe [-r] (--burstein | --dubov | --dutch | --lim) (model-file -g | -g [config-file]) -o trf_file [-s random_seed] [-l [check-list-file]]

If bbpPairings.exe is not in the search path, the path to the executable should
be substituted for bbpPairings.exe. For example, on Unix-based systems, if
bbpPairings.exe is in the current directory, you could replace bbpPairings.exe
with ./bbpPairings.exe

Error codes
-----------
When the program encounters an error, it usually prints a message describing the
error to the standard error stream. (As an exception, an error while generating
a checklist file may be written to the checklist file itself and not reported
elsewhere.) If the program does not complete its main operation because of an
error, the program also returns error codes that a calling program can use to
categorize the cause of the error. The following codes are used in the following
situations:
0: The program completed successfully.
1: The operation could not be completed because no valid pairing exists for the
   current round of the tournament. (This is not considered an error when
   checking a tournament; instead, a message is included in the list of
   discrepancies.)
2: The program encountered an unexpected error.
3: Some part of the request was invalid, for example, a wrongly formatted input
   file.
4: The request may be valid, but the size of the data could not be handled by
   the program, either because it ran out of memory or due to size constraints
   specified at compile time or imposed by the compiler. (This error code is not
   triggered during checklist generation.)
5: There was an error while attempting to access a file; for example, the input
   file might not exist or might not be readable. (This error code is not
   triggered for checklist files.)

Dubov system
------------
The engine for the Dubov system (--dubov) follows C.04.4.1 of the FIDE handbook,
approved by the Council on 28/10/2025 and applied from 1 February 2026. In a
TRF, the tournament type codes FIDE_DUBOV and FIDE_DUBOV_2026 (and their _BAKU
variants) select it.

Each step of the rules is solved with the weighted matching algorithm: the
pairing-allocated bye (Article 3.1), the choice of the upfloaters (Articles
3.2.1 and 3.2.2, criteria C1 to C10), the G1/G2 re-composition (Article 3.2.4)
and the choice of the transposition T2 (Article 3.2.6). When the rules ask for
"the first set" in some order, the candidates are examined in that order and the
choice is frozen as soon as it is compatible with an optimal pairing.

The rules leave a few points open; the engine makes the following choices:
- Only played games count as upfloats (Article 1.8) and for ARO (as Article 1.7
  states). A forfeit is not an upfloat, as for the floats of the Dutch engine.
- C9 (Article 2.3.5) minimises the total number of previous upfloats of the
  maximum upfloaters chosen.
- The "last round" for C8 to C10 is determined from the number of rounds (XXR).
- If the colour difference and the last two colours give opposite absolute
  preferences, the colour difference prevails.
- As in the other engines, TPNs are the effective pairing numbers: players who
  have not yet taken part in any pairing (late entries) are not numbered. This
  matters, for instance, for the colours of round 1 (Article 5.2.1) when a
  player is absent from it.
- Players without a rating are treated as rated 0 (Article 1.1 requires the
  Chief Arbiter to assign one).

Lim system
----------
The engine for the Lim system (--lim) follows C.04.4.3 of the FIDE handbook,
approved by the Council on 28/10/2025 and applied from 1 February 2026. In a
TRF, the tournament type codes FIDE_LIM and FIDE_LIM_2026 (and their _BAKU
variants) select it.

The Lim rules are a procedure, and the engine follows it: the pairing-allocated
bye first (Article 1.1), then the score groups from the highest one down to just
before the Median Scoregroup, from the lowest one up, and the Median Scoregroup
last (Article 2.2); in each score group, the floaters coming from another group
first (Articles 3.6 to 3.8), the transfers of Article 2.3, then the proposed
pairings of Article 2.4 scrutinised with the exchanges of Article 4; a blocked
Median Scoregroup is extended by cracking pairings of its neighbours (Article
2.6). The weighted matching algorithm answers the questions the procedure asks:
every choice keeps the largest possible number of pairs in the score group, and
among those choices, the first one in the order given by the rules.

The rules leave a few points open; the engine makes the following choices:
- The "highest numbered player" is player #1 of the score group (as in Article
  4.2), that is, the smallest pairing number; the "lowest numbered player" is
  the last one.
- Compatibility (Article 2.1) requires that the players did not meet and that a
  colour allocation gives neither of them the same colour three times in a row
  nor three more games with one colour than with the other. In the last round,
  players with the same score are compatible regardless of colours (Article 6).
  If no complete pairing satisfies Article 2.1, only the rule that players do
  not meet twice is kept.
- Only played games count for colours, previous meetings and floats. A player
  floated in the previous round when they played an opponent with a different
  score.
- The bye goes to the lowest ranked eligible player of the lowest score group
  whose removal leaves a complete pairing possible.
- The floater making a score group even (Article 2.3.4) is chosen by, in
  order: the most pairs left in the group (3.10.2), a compatible opponent in the
  adjacent score group (3.9), not having floated in the previous round (3.10),
  equalising the due colours (3.2.2), then the lowest numbered player when
  pairing downward and the highest numbered one when pairing upward (3.2.4,
  3.3). The exclusion of the opponents of other floaters (3.3, 3.4) is not
  modelled.
- A floater is paired with the first player, in the order of Article 3.8, who is
  due the alternate colour, among those keeping the most pairs in the group; if
  no player is due the alternate colour, the first compatible player in that
  order. Outside the Median Scoregroup, a floater without a compatible opponent
  goes on to the next score group (3.5); in the Median Scoregroup, it stays and
  is paired with the others.
- The second scrutiny of Article 5.2 is made together with the first one: in
  scrutiny order, each player receives the first opponent, in the order of
  exchanges of Articles 4.2 and 4.3, who grants both players their due colours,
  if such an opponent keeps the most pairs in the group, and otherwise the
  first opponent that does.
- When the Median Scoregroup is blocked (Article 2.6), the last pairing made in
  the nearest paired score group on the chosen side is cracked.
- If both players are due the same colour and their colour histories are
  identical (Article 5.4), "in the median scoregroup or above" refers to the
  score group in which the pair was made.
- The Maxi-tournament restrictions (Articles 3.2.3, 3.8 and 5.7) are not
  applied.

Time complexity
---------------
Let n denote the largest player ID in a given tournament and r the number of
rounds already played. For simplicity, assume r is O(n).

This program is believed to achieve a theoretical runtime that is O(n^3) for
pairing a single round using the Burstein system.

For the Dubov system, pairing a round is believed to take time O(n^3 * s),
where s is the number of occupied score groups.

For the Lim system, pairing a round is believed to take time O(n^5) in the
worst case (a matching for each player of each score group), and much less in
practice.

For the Dutch system, pairing a round is believed to take time
O(n^3 * s^2 * log n), where s is the number of occupied score groups
in the current round. Note that if the point system (the number of points for
wins and for draws) is treated as constant and there is no acceleration, then s
is O(r), since the point values are rational numbers, and in that case, the
runtime is O(n^3 * r^2 * log n). If we also assume that r is O(log n), then the
runtime is O(n^3 * (log n)^3).

The core of the pairing engine is an application of the simpler of the two
weighted matching algorithms exposited in "An O(EV log V) Algorithm for Finding
a Maximal Weighted Matching in General Graphs," by Zvi Galil, Silvio Micali, and
Harold Gabow, 1986.
