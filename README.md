# MusicGenerator

This program is a solution to challenge https://www.reddit.com/r/dailyprogrammer/comments/7ifbd5/20171208_challenge_343_hard_procedural_music/.

It allows random generation of music driven by initial settings read on standard input. The program can generate one or more pieces with a chords line and a melody line. The chords line will determine the corresponding notes that may be played by the melody line.

A set of one or more notes played simultaneously may get their pitch adjusted by a certain number of octaves, and their order changed to be as much as possible in accordance with the line pitch reference. This reference is initially picked randomly in the range of the minimum/maximum pitches for each line, and then decremented/incremented randomly after each set of notes generated.

The input must follow the below structure:
```
2                     -- Number of pieces generated

-- Piece 1 settings
2                     -- Tonic note (0 = C, 1 = C#, ..., 11 = B)                        
0                     -- Minor flag (0/1)
16                    -- Number of bars
4                     -- Bar size
320                   -- Bar duration
20 20 20 20 20 20 0   -- 5th chord frequencies (example means that a chord has 20/120 chances to be in position I to VI and 0/120 to be in position VII)
0 10 10 0 10 10 0     -- 7th chord frequencies

-- Chords line settings
0                     -- Depth (number of times a beat can be splitted in <bar size> parts)
10 60                 -- Beat split fraction (example means that a beat has 10/70 chances to be splitted and 60/70 to stay at the current depth)
30 60                 -- Beat merge fraction (only possible if previous beat is at the same depth)
0 10 20 30            -- 5th chord richness frequencies (example means that a set has 30/60 chances to be composed of 3 notes, 20/60 to be composed of 2 notes, ...)
0 10 20 30 40         -- 7th chord richness frequencies
30 59                 -- Minimum/Maximum pitches (0 to 127)
30 20                 -- Pitch reference change fraction

-- Melody line settings (same structure as chords line)
1                     -- Depth must be greater than or equal to chords line depth
10 60
30 60
0 30 20 10
0 40 30 20 10
60 89                 -- Minimum/Maximum pitches must not overlap those of chords line
30 20

-- Piece 2 settings
...

-- Index of pieces generated in output (0-based)
0
1
0
```
The output generated has the below structure (1 line per note played):
```
...
81 48.80 0.80         -- Pitch/Start/Duration (start and duration are divided by 100)
73 49.60 0.80
74 50.40 0.40
78 50.40 0.40
74 50.80 0.20
83 50.80 0.20
...
```
The output may be played in page http://ufx.space/stuff/procmusic/?tempo=120&wavetype=square&volume=0.1 (credit https://www.reddit.com/user/Cosmologicon).
