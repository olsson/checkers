#!/usr/bin/perl -w
#
# Test program to run two checkers programs against each other
#
# Assumes that the two programs are specified as the first two arguments 
# on STDIN

use FileHandle;
use IPC::Open2;

sub usage { print "Usage: ./compare.pl <prog1> <prog2>\n"; exit; }

usage if ($#ARGV != 1);

$SIG{'PIPE'} = 'failure';

open2(\*IN1, \*OUT1, $ARGV[0]);
open2(\*IN2, \*OUT2, $ARGV[1]);

OUT1->autoflush();
OUT2->autoflush();

# make prog1 black and prog2 white
print OUT1 "b\n";
print OUT2 "w\n";

# wait for prog2 to be ready for input
while (<IN2>) { last if (/Opponent\'s move was:/); }

# play the game
while (1) {
    read_move("b", \*IN1, \*OUT2) or last;
    read_move("w", \*IN2, \*OUT1) or last;
}

# read a move, print some statistics and write the move
sub read_move {
    my $color = shift;
    my $in = shift;
    my $out = shift;
    my $got_move = 0;
    my $write_board = 0;
    my ($depths, $move, $time, $total_time, $standings);
    

    while (<$in>) {
	last if (/Opponent\'s move was:/);
	if ($write_board) {
	    
	    print $_;
	    $write_board = 0 if (/---------------/);
	    next;
	}
	if (/(\d+) depths,/) {
	    $depths = $1;
	    next;
	}
	if (/My move is: \d+. (\d+(-\d+)+)/) {
	    $move = $1;
	    next;
	}
	if (/Time on this move: (\d+\.\d+)/) {
	    $time = $1;
	    next;
	}
	if (/Total time spent: (\d+\.\d+)/) {
	    $total_time = $1;
	    next;
	}
	if (/Standings: (.*)$/) { 
	    $standings = $1;
	    $got_move = 1;
	    $write_board = 1;
	}
    }

    return 0 if (!$got_move);

    print "$color: $move $depths depths, ${time}s ${total_time}s $standings\n";
    print $out "$move\n";
    print $out "y\n";

    return 1;
}





