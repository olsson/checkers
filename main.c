#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "checkers.h"

/*
 *	Rødgrød mit grädde, a Checkers program
 *	(c) 2004 Lunds Tekniska Högskola
 *	
 *	Game Management:  Erik Olsson       eolsson@ics.uci.edu
 *	Evaluation:       Joachim Raue      raue@gmx.net
 *	Search:           Matthew Wytock    mwytock@ucsd.edu
 *
 */
 
 
/* function prototypes */
void my_turn();								///< Takes the system's turn.
bool okay();									///< Asks operator to verify new board.
bool accept_draw();						///< Determines if the system accepts a draw offer.
int opponent_turn();					///< Asks for the opponent's turn.
int my_pieces_left();					///< The number of pieces the system has left.
int opponent_pieces_left();		///< The number of pieces the opponent has left.
int pieces_left(bool color);	///< The number of pieces left for 'color'.
unsigned int how_much_time();	///< Decides how much time should be given to decide what the best move is.

/* global variables */
double secs_left = 300; 			//!< CPU seconds left for the system 
double time_used = 0;					//!< CPU seconds used for the system (clock_t)
int moves = 0; 								//!< Moves made by the system		
int	total_moves = 0; 					//!< Total moves made by both players
int last_forty = 40; 					//!< Counter for the last (limits time when down to one man)
int	score = 0;								//!< The score of the most recent board
char move_list[300][128];     //!< Keeps a list of all moves made during the game (if logging)
char *logfile;	              //!< Points to the filename for the logfile
bool color;                   //!< Which side the system is playing as
bool logging = FALSE;         //!< Whether or not we're logging this game

/* game boards */
bitboard backb[N_BOARDS];			//!< Backup board
bitboard board[N_BOARDS];			//!< Main game board

/* alarm handling */
sigset_t alarm_set;           //!< Alarm-handling variable 
jmp_buf env;                  //!< Alarm-handling variable


int main(int argc, char *argv[]) {

  char w, *filename = "starts/initial.wdp";		//	initial board is the default 
  struct move list_moves[MAX_MOVES];
  bool c = BLACK;
  int i;

  printf("Rødgrød mit grädde - Checkers\n(c) 2004 Lunds Tekniska Högskola\n\n");

	
	/* command-line options */
  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0) {
      printf("Usage: ./checkers [-lt] [board-file] [log-file]\n");
      printf("          -l  Log this game to specified file.\n");
      printf("          -t  Ignore time constraints\n");
      printf("       To specify a board path and a log path at the same\n");
      printf("       time, omit both -l and t.\n"); 
      printf("       Omitting all arguments starts a new game at the \n");
      printf("       standard position with no logging.\n");
      return 0;
    }
    if (strstr(argv[1], "-") != NULL) {
      if (strstr(argv[1], "l") != NULL) {
				if (argc == 3) {
          logfile = argv[2];
          printf("Logging game to %s.\n", logfile);
          logging = TRUE;
        }
				else {
          printf("Invalid command!\nType './checkers --help' for command line help.\n");
          return 0;
        }
      }
      if (strstr(argv[1], "t") != NULL) {
        printf("Unlimited time.\n");
				secs_left = (double)INFINITY;
        if (logging == FALSE && argc == 3) {
          filename = argv[2];
          printf("Using %s instead.\n", filename);
        }
      }
    }
    else if (argc == 2) {
      filename = argv[1];
      printf("Using %s instead.\n", filename);
    }
		else if (argc == 3) {
			filename = argv[1];
      logfile = argv[2];
      printf("Using %s instead.\n", filename);
      printf("Logging game to %s.\n", logfile);
      logging = TRUE;
		}	
  }
  printf("\n");

	/* operator selects which side the computer shall play as */
  while (w != 'b' && w != 'w') {
    printf("OPERATOR: Enter 'b' if system is black, 'w' if white. [b/w] ");
    fflush(stdout);
    w = getc(stdin);
  }
  if (w == 'b')
    color = BLACK;
  else
    color = WHITE;
  
  // consume extra token -- for some reason an extra character gets entered in 
	//	the getc() operation above.
  w = getc(stdin);
  
  
  // load initial board, ignores position by using the dummy variable c
  if (!read_wdp(board, filename, &c, &secs_left)) {
    fprintf(stderr, "Could not load board map!\n");
    return 0;
  } 

  /* set up handling of alarm signal */
  sigemptyset(&alarm_set);
  sigaddset(&alarm_set, SIGALRM);


  // print the board that has been loaded
	#ifdef DEBUG
  printf("printing board in initial position\n");
  print_board(board);
  printf("%s has %d pieces.\n%s has %d pieces.\n", (color ? "Black" : "White"), 
		my_pieces_left(), (!color ? "Black" : "White"), opponent_pieces_left());
	#endif  

	
  // if i'm white, i wait for my opponent, since black starts every game
	// 	in other words, i enter the main game loop halfway
  if (color != c) {
    print_board(board);
    goto _WHT;
  }
	// main game loop
  while (1) {
    printf("\n\n%s's turn:\n\n", (color ? "Black" : "White"));
		
		// check to see if i can even make any moves, if no, i lose
    if (generate_moves(board, (bool)color, list_moves) <= 0) {
      printf("\nI cannot make any moves!\n");
      printf("\nI lost the game.\n\n");
      break;
    }
		// check to see if my opponent can make any moves, if not, i win
    if (generate_moves(board, (bool)!color, list_moves) <= 0) {
      printf("\nOpponent cannot make any moves!\n");
      printf("\nI won the game.\n\n");
      break;
    }

		// take my turn
    my_turn();
    
		// if my time ran out while taking my move, i lose
    if (secs_left <= 0) {
      printf("\nMy time ran out, so I give up!\n");
      printf("\nI lost the game.\n\n");
      break;
    }
		
		// everything went ok, so print the board
    print_board(board);

		// write to the log, if thats what we want to do
    if (logging)
      write_log(board, move_list, logfile, total_moves, !&color, secs_left);
    
		// maybe we jumped the last piece? if so, we won
		if (opponent_pieces_left() <= 0) {
      printf("\nI won the game.\n\n");
      break;
    }
		
		// now it's the "white's" turn -- depends on start order
  _WHT:
	
		// backup the current board
		for (i = 0; i < N_BOARDS; i++)
      backb[i] = board[i];
	
		printf("\n\n%s's turn:\n\n", (!color ? "Black" : "White"));
		
		// check to see if the opponent can make moves, if not system wins
    if (generate_moves(board, (bool)!color, list_moves) <= 0) {
      printf("\nOpponent cannot make any moves!\n");
      printf("\nI won the game.\n\n");
      break;
    }
  
		// "backup" loop
    while (1) {
			// opponent "takes his turn"; returns something > 1 if a draw was offered
      i = opponent_turn();

      if (i == 2) {
				printf("Draw has been offered and accepted.\n");
				printf("\nThe game ended in a draw.\n\n");
				return 0;
      }
      else if (i == 3) {
				printf("Draw has been offered and refused. Game will continue.\n");
				goto _WHT;
      }
      else if (i == 4) {
        printf("\nIllegal move by %s! Should have jumped.\n", 
               (!color ? "Black" : "White"));
        printf("\nI won the game.\n\n");
        return 0;
      }
			// print the board for visual verification that move was entered correctly
      print_board(board);

      // back funtionality asks the operator if what he entered looks right
			//	if he says it isnt right, he gets kicked back one step to re-enter 
			//	the move
      if (okay()) {
				break;
      }
      else {
				for (i = 0; i < N_BOARDS; i++)
					board[i] = backb[i];
      }
    }
    
		// write to the log, if thats what we want to do
    if (logging)
      write_log(board, move_list, logfile, total_moves, &color, secs_left);

		// opponents move could have taken my last piece. if so, i lost
    if (my_pieces_left() <= 0) {
      printf("\nI lost the game.\n\n");
      break;
    }
  }
  return 0;
}


/**
 *	Nags the operator until they enter either 'y' or 'n', signifying
 *	whether or not the position he entered earlier is correct.
 *
 *	\return TRUE if the operator says that the board is correct.	
 *          FALSE if the operator says no, and wants to change the move.
 */
bool okay() {
  char inpt[1];
	
  while((inpt[0] != 'y' && inpt[0] != 'n') || inpt[0] == 0) { 
    printf("OPERATOR: Is the board correct? [y/n] ");
    fflush(stdout);
    gets(inpt);
    if (inpt[0] == 'y')
      return TRUE;
    if (inpt[0] == 'n')
      return FALSE;
  }
  return FALSE;
}

/** 
 *
 *	Goes through the neccessary motions to get the information about what 
 *	the opponent's move was.
 *
 *	\return	1 - if turn was executed normally,  
 *					2 - if a draw was offered and accepted, 
 *					3 - if a draw was offered and refused, 
 *          4 - if a jump was possible and the opponent did not take the jump.
 *	
 */
int opponent_turn() {

  // if opponent went first, ask to get that move:

  char ctrl = (int)NULL, inp[128], inpt[1];
  struct move opp_move;
  int n = 0, from, to, diff;
  struct move l_moves[MAX_MOVES];
  bool must_jump = FALSE, once = TRUE, nagflag;

	// get the move string
  while (!ctrl) {
    nagflag = TRUE;
    
    // checks to see if there are any 
    n = try_capture(board, 0xffffffff, !(int)color, l_moves);
      
    if (n > 0 && once) {
      printf("\nWarning: %s has %d possible jump%s.\n", (!color ? "Black" : "White"), 
               n, (n > 1 ? "s" : ""));
      printf("         %s must jump or %s will lose!\n\n",
              (!color ? "Black" : "White"), (!color ? "Black" : "White"));
      must_jump = TRUE;
      once = FALSE;
    }    
    
    printf("OPERATOR: Opponent's move was: \n");
    //fflush(stdout);
    if (gets(inp) == NULL) {
      ctrl = (int)NULL;
		}

		// if the keyword "draw" was entered, decide whether or not to accept it
    if (strcmp(inp, "draw") == 0) {
      printf("Opponent has offered a draw.\n");
      if (accept_draw() == TRUE) {
				return 2;
      }
      else {
				return 3;
      }
    }
		// for all other input check to see if it is "valid"
    else if (valid_move_input(inp)) {
			                      
      // valid? ok cool, then convert the move string into a move on the board
      
      if ((trans_string_move(board, &opp_move, inp, !color)) != FALSE) {
        
        if (inp && sscanf(inp, "%d-%d", &from, &to) >= 2) {
          from--;to--; 
          
          diff = (from/4+1)-(to/4+1);
          if (must_jump && (diff > -2 && diff < 2)) {
            printf("\nWarning: This move will cause %s to lose!\n", (!color ? "Black" : "White"));
          
            while((inpt[0] != 'y' && inpt[0] != 'n') || inpt[0] == 0) { 
              printf("\nOPERATOR: Are you sure this is correct? [y/n] ");
              fflush(stdout);
              gets(inpt);
              if (inpt[0] == 'y')
                return 4;
              if (inpt[0] == 'n') {
                inpt[0] = (int)NULL;
                nagflag = FALSE;
                break;
              }
            }
            
          }
          
          else {
            ctrl = 1;
            // update the main board to reflect the opponent's move
            board[WHITE] = opp_move.board[WHITE];
            board[BLACK] = opp_move.board[BLACK];
            board[KING] = opp_move.board[KING];

            // if we're logging, enter this move into the move_list array
            if (logging) 
              strcpy(move_list[total_moves++], inp);
			
            break;
          }
        }
      }
    }
    if (nagflag)
      printf("Invalid move entered!\n");
    ctrl = (int)NULL;  
  }
  return 1;
}

/**
 *	Goes through the neccessary motions to decide upon and make 
 *	the best possible move given the current board.
 */
void my_turn() {

  clock_t t = 0;							// "reset" the clock each time  
  double dt;
  unsigned int use;						// how much time we get to use
  struct move best_move;			// stores the move decided upon
  char move_str[128],					// stores the textual representation of the move 
			 *stand;	 							// the "standings" string

  t = clock();  							// start counting time from NOW!
	
  use = how_much_time();			// decide how much time use for this move			

  if (setjmp(env) == 0)				// starts the alarm timer then find the best move!
    mtdf(board, &best_move, color, use);	

  moves++;
  
  t = clock();								// stop counting time NOW!
  t = t - time_used;					// since clock reports the time since it was FIRST 
  time_used += t;							//	called ever in this program, we need to keep tabs
															// 	on all the time we've used so far to get the exact 
															// 	time for this move

	// convert the new move to a string to display
  trans_move_string(board, &best_move, move_str, color);
  printf("       My move is: %d. %s\n", moves, move_str);

	// copy the new move to the main game board
  board[BLACK] = best_move.board[BLACK];
  board[WHITE] = best_move.board[WHITE];
  board[KING] = best_move.board[KING];

	// log this move, if we want to
  if (logging) 
    strcpy(move_list[total_moves++], move_str);

	// figure out some stuff to get the right time
  if (t != -1) {
    dt = (double) t / (double) CLOCKS_PER_SEC;
    secs_left = secs_left - dt;
    printf("Time on this move: %.2lf\n", dt);
  }
  
  printf(" Total time spent: %.2lf\n", 300 - secs_left);
  
	// find out how much the new game board is worth 
  score = eval(board, !color);
	
	// then assign a string to describe the standings more roughly
  if (score < -350 || score > 350) 
    stand = (score < 0) ? "White is winning" : "Black is winning";
  else if (score <= -200 || score >= 200)
    stand = (score < 0) ? "Very large white advantage" : "Very large black advantage";  
  else if (score <= -100 || score >= 100)
    stand = (score < 0) ? "Large white advantage" : "Large black advantage";
  else if (score <= -30 || score >= 30)
    stand = (score < 0) ? "Medium white advantage" : "Medium black advantage";
  else if (score > -30 && score < 30 && score != 0)
    stand = (score < 0) ? "Slight white advantage" : "Slight black advantage";
  else if (score == 0)
    stand = "Equal";
  else 
    stand = "";

  printf("        Standings: %s (%d)\n", stand, score);

}

/**
 *	Determines, based upon material and score standings, whether 
 *	or not to accept a draw offer by the opponent.
 *
 *	\return	TRUE if the draw is accepted, 
 *					FALSE if the draw is refused.
 */
bool accept_draw() {

  int mp = my_pieces_left(), op = opponent_pieces_left();

  // if i have less pieces and a worse score than the other guy, accept the draw
  if (mp < op && (color ? score < 0 : score > 0) ) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/**
 *	Determines, based upon material, score standings, move possibilities,  
 *	and remaining time, how much time should be allotted for the current move.
 *
 *	\return	The allotted time in whole seconds.
 */
unsigned int how_much_time() {

  int r = 0, time, p = my_pieces_left(), t = opponent_pieces_left();  
	struct move m_list[MAX_MOVES];
	
	// check to see how many moves are possible. if only one is possible, then
	// allow as little time as possible for that move, since what happens after-
	// wards doesn't matter if you only have one first move choice.
	if (generate_moves(board, (bool)color, m_list) == 1)
		return 1;
	
  // if BLACK & Good score  or  WHITE & Good score, then give less time
  if (color ? score > 0 : score < 0) { 
    r = 0;
  } 
  else {  
    // defensive, more time to think since our last score was not too good 
    r = 2;
  }

	// how many pieces do i have left?
  switch (p) {
		case 12:
		case 11:
    	time = 1;
			break;
		case 10:
		case 9:
		case 8:
			time = 3+r;
			break;
		case 7:
		case 6:
		case 5:
		case 4:
    	time = 5+r;
			break;
		case 3:
		case 2:
    	time = 8+r;
			break;
		default:
    	if (t == 1 && p == 1) {
				// allow for at least 40 moves when only one of each piece is left
				time = (int)secs_left / last_forty--;
			}
			else {
				// i have 1 piece left and they have more than one, which means i'll 
      	// probably lose, so divide up the remaining time in somewhat big chunks
				time = (int)secs_left / last_forty;
			}
  }

	// if we're giving the system more time than is left, just give it all the time
  if ((double)time >= secs_left)
    time = (int)secs_left;
  
	// if we're giving the system time less than a second, round up to 1 second
	//	otherwise we'll crash when we pass "0" seconds'
  if (time > secs_left / 30)
    time = secs_left / 30;
  if (time < 1)
    time = 1;
  
  return time;
}    

/**
 *  Determines how many pieces of the specified color are left on the board.
 *
 *	\param color The color to check how many pieces are left.
 *	\return The number of pieces.
 */
int pieces_left(bool color) {
  bitboard to, next;
  int n = 0;
  
  to = board[(int)color];
  while (to) {
    next = LAST_ONE(to);
    n++;
    to ^= next;
  }
  return n;
}

int my_pieces_left() {
  return pieces_left(color);
}

int opponent_pieces_left() {
  return pieces_left(!color);
}

/**  end of main.c  **/
