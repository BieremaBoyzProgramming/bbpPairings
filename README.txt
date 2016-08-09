BBP Pairings, a Swiss-system chess tournament engine
created by Bierema Boyz Programming
copyright (C) 2016
See LICENSE.txt for additional copyright and licensing information.

The most recent version of this program can be downloaded at
https://github.com/BieremaBoyzProgramming/bbpPairings/releases .


BBP Pairings is an engine for pairing players in a Swiss-system chess
tournament. It attempts to implement rules specified by FIDE's Systems of
Pairings and Program Commission. To be clear, the program is not endorsed by
FIDE or the SPP. It is not a full tournament manager, just an engine for
computing the pairings.

The program currently implements only the Burstein system.

The program's interface is designed to be very similar to that described in the
advanced user manual for JaVaFo 1.4 (with permission to do so from the author,
Roberto Ricca). The remainder of this document lists the points of divergence.
For more informaton about JaVaFo, please visit www.rrweb.org/javafo . This
documentation assumes full understanding of the JaVaFo AUM.

Hereafter, we will use the name "JaVaFo" to refer to JaVaFo 1.4.

TRF(xb)
-------
BBP Pairings supports only one file format, TRF(xb), an extension of the TRF(x)
format defined for JaVaFo.

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

For this reason, we must introduce an extension to the TRF(xb) format. The
extension uses codes BBW and BBD for specifying the number of points awarded for
a win and a draw, respectively. The format is
BBW pp.p
BBD pp.p
Thus, for a tournament with 3 points for wins and 1 point for draws, the TRF(xb)
should contain the lines
BBW  3.0
BBD  1.0

If the tournament uses the standard point system (as of the 2014 rules), that
is, 1 point for wins and 0.5 points for draws, these lines are not necessary.

Because this requirement causes an incompatibility with the TRF(x)
representation of non-standard point system tournaments that could be overlooked
when exchanging files, BBP Pairings checks that all players' scores are computed
correctly from the tournament results. If this is not the case, it issues an
error message and refuses to proceed.

Future versions may relax the strict requirement to permit point systems that
are linear multiples of the standard system, such as the 2-1-0 point system
currently being discussed.


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


Generator
---------
BBP Pairings introduces a new option for generating tournaments paired using the
Burstein system. Since the Burstein system has a specific acceleration system
included in its rules, the "Accelerated" option instructs the generator to apply
these rules when pairing the tournament. To turn on this option, include the
line
Accelerated=1
in the configuration file.

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

BBP Pairing has the additional option to generate checklist files while
generating or checking tournaments, not just when pairing individual rounds. In
this case, the tables for all of the rounds are put into the same file, with
round number indications appearing between them.

Command line arguments
----------------------
BBP Pairings always performs pairings using a weighted matching algorithm, so it
does not use the -w and -q options of JaVaFo.

Since BBP Pairings has been designed with the intention of supporting more than
one Swiss system in the future, an argument specifying which Swiss system to use
for pairing is required.

For the most part, BBP Pairings was written to be compatible with compilers
supporting the 2014 C++ standard (C++14). However, C++14 has no portable way to
perform file path manipulation as is performed by JaVaFo (where the output file
is created in the same directory as the input file if no directory is
specified). Thus, by default, BBP Pairings does not have this capability, and
the directory must be specified for both the input and output files if needed.
However, this is included as a new feature in the working draft of the upcoming
C++ standard (C++17), and if BBP Pairings is linked with a library that supports
the new API, BBP Pairings will behave like JaVaFo in this respect (assuming that
the proper flags were enabled when compiling BBP Pairings).

If file path manipulation is not supported by a build, this is mentioned in the
version information displayed when using the -r flag.

The acceptable syntax forms for running BBP Pairings are:
bbpPairings.exe [-r]
bbpPairings.exe [-r] --burstein input-file -c [-l check-list-file]
bbpPairings.exe [-r] --burstein input-file -p [output-file] [-l check-list-file]
bbpPairings.exe [-r] --burstein (model-file -g | -g [config-file]) -o trf_file [-s random_seed] [-l check-list-file]

On a build of BBP Pairings that supports file path manipulation capabilities,
the check-list-file argument is optional, with the same default as JaVaFo. If no
directory is given and file path manipulation is supported, the checklist file
is written to the same directory as input-file or trf_file (whichever is
present).

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
Let n denote the largest player ID in a given tournament. Assuming the number of
rounds in the tournament is O(n), this program is believed to achieve a
theoretic runtime that is O(n^3) for pairing a single round using the Burstein
system.
