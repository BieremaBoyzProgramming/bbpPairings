BBP Pairings, a Swiss-system chess tournament engine
created by Bierema Boyz Programming
See LICENSE.txt for copyright and licensing information.

The most recent version of this program can be downloaded at
<https://github.com/BieremaBoyzProgramming/bbpPairings/releases>.


BBP Pairings is an engine for pairing players in a Swiss-system chess
tournament. It attempts to implement rules specified by FIDE's Systems of
Pairings and Program Commission. It is not a full tournament manager, just an
engine for computing the pairings.

The program currently implements the 2017 rules for the Dutch system. It also
includes a flawed implementation of a version of the Burstein system, not
endorsed by the SPP.

The program's interface is designed to be very similar to that described in the
advanced user manual for JaVaFo 1.4 (with permission to do so from the author,
Roberto Ricca). The remainder of this document lists the points of divergence.
For more informaton about JaVaFo, please visit <www.rrweb.org/javafo>. This
documentation assumes full understanding of the JaVaFo AUM.

Hereafter, we will use the name "JaVaFo" to refer to JaVaFo 1.4.

TRF(bx)
-------
BBP Pairings supports only one file format, TRF(bx), an extension of the TRF(x)
format defined for JaVaFo.

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


BBP Pairings outputs files using the codes introduced in the 2016 version of the
TRF, but it can also read files produced using the codes specified in the JaVaFo
AUM.


Random Tournament Generator
---------------------------
The point value parameters not supported in JaVaFo 1.4 can be set using the keys
PointsForLoss, PointsForZPB, PointsForForfeitLoss, and PointsForPAB. All of
these have default values of 0.0, except the last, which by default is set equal
to PointsForWin.

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
for the pairing-allocated bye (Y if yes, N if no). The C12 and C14 columns
indicate the floating direction of the player on the previous and
next-to-previous rounds, respectively (U for upfloat, D for downfloat).

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
bbpPairings.exe [-r] (--burstein | --dutch) input-file -c [-l check-list-file]
bbpPairings.exe [-r] (--burstein | --dutch) input-file -p [output-file] [-l check-list-file]
bbpPairings.exe [-r] (--burstein | --dutch) (model-file -g | -g [config-file]) -o trf_file [-s random_seed] [-l check-list-file]

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

Time complexity
---------------
Let n denote the largest player ID in a given tournament and r the number of
rounds already played. For simplicity, assume r is O(n).

This program is believed to achieve a theoretical runtime that is O(n^3) for
pairing a single round using the Burstein system.

For the Dutch system, pairing a round is believed to take time
O(n^3 * s * (d + s) * log n), where s is the number of occupied score groups
in the current round and d is the number of distinct score differences between
two players in the round. Note that if the point system (the number of points
for wins and for draws) is treated as constant and there is no acceleration,
then d and s are both O(r), since the point values are rational numbers, and in
that case, the runtime is O(n^3 * r^2 * log n). If we also assume that r is
O(log n), then the runtime is O(n^3 * (log n)^3).

The core of the pairing engine is an application of the simpler of the two
weighted matching algorithms exposited in "An O(EV log V) Algorithm for Finding
a Maximal Weighted Matching in General Graphs," by Zvi Galil, Silvio Micali, and
Harold Gabow, 1986.
