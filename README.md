# picotree

This parses a directory of csv files produced by a Picoscope and turns them into a ROOT TTree.  It works for me... at least with version 7.0.110 of the Picoscope and ROOT 6.26.06.

ps->Draw("cha:t","file==0")

will draw the waveform in channel A from the first file.
