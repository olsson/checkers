#include <stdio.h>
#include <string.h>
#include "checkers.h"


/**
 * Evaluates an integer variable to a board position stating to whose favor
 * the board position is considered.
 *
 * \param board Board configuration
 * \param btm The color whose turn it is ("black to move")
 * \return An integer. Positive values express an advantage for black and vv.
 */
int eval(bitboard *board, bool btm)
{
  const bitboard empty = ~(board[WHITE] | board[BLACK]);
  
  /* The factors whose sum will be returned */
  int material_val=0,   /* Material */
      trapped_val=0,    /* Trapped Kings */
      doghole_val=0,    /* Dog Holes */
      safe_val=0,       /* Safe Bricks */
      runaway_val=0,    /* Runaway Men */
      turn_val=0,       /* Turn */
      backrank_val=0,   /* Weak Backrank */
      kills_val=0;      /* Possible Kills */

  bitboard to = board[BLACK]|board[WHITE];
  bitboard next=0;      /* mask to the current brick */
  bool color=0;         /* color of the current brick */
  int color_bias=0;     /* -1 for white, 1 for black - useful for adding or
                           subtracting values depending on the brick's color */

	int i=0;							/* loop variable for the detection of next fields */
	bitboard next_moves;	/* mask to the possible next moves of a king */
	bitboard up;          /* mask to one upper neighboring field */
	bitboard down;        /* mask to one lower neighboring field */
  int8 currway=0;       /* runaway analysis for the current brick */
  bitboard rest[N_BOARDS];

  #define EXTRA 2
  int8 curkills=0;      /* possible kills for current brick */
  int8 moves=0;         /* direction in which current brick can move */
  int8 kills[4];
  kills[WHITE]=0;       /* total kill possibilities white bricks have */
  kills[BLACK]=0;       /* total kill possibilities black bricks have */
  kills[WHITE+EXTRA]=0; /* max. extra kills white bricks have */
  kills[BLACK+EXTRA]=0; /* max. extra kills black bricks have */

  /* one loop through all bricks is faster than many loops */
  while (to) {
    next = LAST_ONE(to);
    
    /* set values according to brick color */
    if (next & board[BLACK]) {
      color = BLACK;
      color_bias = 1;
      if (next & board[KING])
        moves = UP + DOWN;
      else
        moves = DOWN;
    } else {
      color = WHITE;
      color_bias = -1;
      if (next & board[KING])
        moves = UP + DOWN;
      else
        moves = UP;
    }

    /* Possible Kills */
    curkills = poss_kills(board, color, moves, next, 0, 0);
    if (curkills > 0) {
      #ifdef DEBUG_EVAL_DETAILS
      printf("- Possible kills for field %d: %d\n", fieldnumber(next), curkills);
      #endif
      kills[(int)color]++;
      if (curkills-1 > kills[color + EXTRA]) kills[color + EXTRA] = curkills-1;
    }

    if (next & board[KING]) {

      /* Material */
      material_val += color_bias * MAT_KING;
      
      /* Trapped Kings */
      /* Compute a mask with possible next moves for the king first */
			next_moves = 0;
			for (i = 0; i < N_DIRS; i++) { /* Check both directions (NW/SE & NE/SW) */
				/* Compute neighbor possitions */
				down = DOWN_NEIGHBOR(next, i);
  			up = UP_NEIGHBOR(next, i);
      		
   			/* If the neighboring field is empty or occupied by an enemy
   				 which can be killed add the appropriate target field      */
			  if (down & empty) {
			 		next_moves |= down;
		 		} else {
		 		  if ((down & board[ENEMY(color)]) && (DOWN_NEIGHBOR(down, i) & empty))
		 		  	next_moves |= DOWN_NEIGHBOR(down, i);
		    }
			  if (up & empty) {
			 		next_moves |= up;
		 		} else {
		 		  if ((up & board[ENEMY(color)]) && (UP_NEIGHBOR(up, i) & empty))
		 		  	next_moves |= UP_NEIGHBOR(up, i);
		    }
	    }
         
      /* In order *not* to be trapped there must exist a possible kill or
      	 one of the poss. moves is either empty or not enemy-threatened   */
		  if ((curkills == 0) &&
          !(next_moves &
            empty & ~(threatened(board, T_ENEMY(T_COLOR(color))) ) )) {
  
          #ifdef DEBUG_EVAL_DETAILS
          printf("- Trapped king on position %d\n", fieldnumber(next));
          #endif
        trapped_val -= color_bias * TRAPPED_KING;
      }

    } else {

      /* Material */
      material_val += color_bias * MAT_MAN;
      
      /* Safe men on the right or left border are a plus */
      if (next & 0x18181818) { // Men on pos. 4, 5, 12, 13, 20, 21, 28 or 29
        #ifdef DEBUG_EVAL_DETAILS
          printf("- Safe man on position %d\n", fieldnumber(next));
        #endif
        safe_val += color_bias * SAFE_MAN;
      }
      
      /* Runaway Checkers */
      /* The runaway function needs the board without the current man in order
         to avoid a wrong result in certain situations */
      rest[BLACK] = board[BLACK] & ~next;
      rest[WHITE] = board[WHITE] & ~next;
      rest[KING] = board[KING] & ~next;
      
      currway = runaway(rest, next, color);
      rest[WHITE] = board[WHITE] & ~next;
      rest[BLACK] = board[BLACK] & ~next;
      rest[KING] = board[KING] & ~next;

      currway = runaway(rest, next, color);
      if (currway > -1) {
        #ifdef DEBUG_EVAL_DETAILS
        printf("- Man on field %d can become king within %d moves.\n",
               fieldnumber(next), currway);
        #endif 
        runaway_val += color_bias * (RUNAWAY_MAN - currway * RUNAWAY_ROW);
      }

    }

    to ^= next;
  }
  
  /* Check for victory */
  if (board[WHITE] == 0) material_val += MAT_VICTORY;
  if (board[BLACK] == 0) material_val -= MAT_VICTORY;

  /* Whose turn is it? */
  float kill_fact[2];
  if (btm) {
    turn_val += 3;
    kill_fact[BLACK] = KILL_FACT_TURN;
    kill_fact[WHITE] = KILL_FACT_NOTURN;
  } else {
    turn_val -= 3;
    kill_fact[BLACK] = KILL_FACT_NOTURN;
    kill_fact[WHITE] = KILL_FACT_TURN;
  }
  
  /* Compute possible kills values */
  if ((kills[BLACK] > 0)) {
    kills_val += kill_fact[BLACK] * POSS_KILL;
    kills_val += kill_fact[BLACK] * POSS_ALTKILL * (kills[BLACK]-1);
    kills_val += kill_fact[BLACK] * POSS_EXTRAKILL * kills[BLACK+EXTRA];
  }
  if ((kills[WHITE] > 0)) {
    kills_val -= kill_fact[WHITE] * POSS_KILL;
    kills_val -= kill_fact[WHITE] * POSS_ALTKILL * (kills[WHITE]-1);
    kills_val -= kill_fact[WHITE] * POSS_EXTRAKILL * kills[WHITE+EXTRA];
  }

  /* Check for weak back ranks */
  int backbricks=0;  
  to = board[WHITE] & king_bits[BLACK];
  while (to) {
    next = LAST_ONE(to);
    backbricks++;
    to ^= next;
  }
  switch(backbricks) {
  case 0:
    backrank_val += BACK_0;
    break;
  case 1:
    backrank_val += BACK_1;
    break;
  case 2:
    backrank_val += BACK_2;
    break;
  case 3:
    backrank_val += BACK_3;
    break;
  }
  backbricks=0;
  to = board[BLACK] & king_bits[WHITE];
  while (to) {
    next = LAST_ONE(to);
    backbricks++;
    to ^= next;
  }
  switch(backbricks) {
  case 0:
    backrank_val -= BACK_0;
    break;
  case 1:
    backrank_val -= BACK_1;
    break;
  case 2:
    backrank_val -= BACK_2;
    break;
  case 3:
    backrank_val -= BACK_3;
    break;
  }
  
  /* Dog Holes */
  if ((board[WHITE] & 0x00000010) && (board[BLACK] & 0x00000001)) {
    // White men on pos. 5 and black brick on pos. 1
    #ifdef DEBUG_EVAL_DETAILS
    printf("- White man in dog hole.\n");
    #endif
    doghole_val += DOG_HOLE;
  } 
  if ((board[BLACK] & 0x08000000) && (board[WHITE] & 0x80000000)) {
    // Black men on pos. 28 and white brick on pos. 31
    #ifdef DEBUG_EVAL_DETAILS
    printf("- Black man in dog hole.\n");
    #endif
    doghole_val -= DOG_HOLE;
  }
  
  /* Output the parameters */
  #ifdef DEBUG_EVAL
  printf("Material: %d\n", material_val);
  printf("Trapped Kings: %d\n", trapped_val);
  printf("Dog holes: %d\n", doghole_val);
  printf("Safe men: %d\n", safe_val);
  printf("Runaway Checkers: %d\n", runaway_val);
  printf("Current Turn: %d\n", turn_val);
  printf("Back Rank: %d\n", backrank_val);
  printf("Possible Kills: %d (B: %d poss., max. %d; W: %d poss., max. %d)\n",
         kills_val, kills[BLACK], kills[BLACK+EXTRA]+1, kills[WHITE], kills[WHITE+EXTRA]+1);
  #endif

  return material_val + trapped_val + doghole_val + safe_val + runaway_val +
         turn_val + backrank_val + kills_val;
}


/**
 * Computes a mask with those fields marked which are threatened by bricks
 * of a certain color.
 *
 * \param board Board configuration
 * \param t_color Color of bricks that are considered as threats
 *                (T_WHITE, T_BLACK, T_BOTH)
 * \return The mask
 */

bitboard threatened(bitboard *board, int8 t_color)
{
  const bitboard empty = ~(board[WHITE] | board[BLACK]);
  
  bitboard retmask = 0; /* Mask to return */
  
  /* Check just bricks of the appropriate color */
  bitboard to;
  if (t_color & T_BLACK) to |= board[BLACK];
  if (t_color & T_WHITE) to |= board[WHITE];
  
  bitboard next=0, down=0, up=0;
  int i=0;

  while (to) {
    next = LAST_ONE(to);
    
    for (i = 0; i < N_DIRS; i++) { /* Check both directions (NW/SE & NE/SW) */
    
      /* Determine neighbors */
      down = DOWN_NEIGHBOR((next&board[BLACK]) | (next&board[WHITE]&board[KING]), i);
      up = UP_NEIGHBOR((next&board[BLACK]&board[KING]) | (next&board[WHITE]), i);
      
      /* A field is considered "threatened" if it is occupied by a hostile
         brick and the next field into the same direction is empty         */
      if (((next & board[BLACK] && down & ~board[BLACK]) ||
           (next & board[WHITE] && down & ~board[WHITE])) &&
          (DOWN_NEIGHBOR(down, i) & empty))
        retmask |= down;
      if (((next & board[BLACK] && up & ~board[BLACK]) ||
           (next & board[WHITE] && up & ~board[WHITE])) &&
          (UP_NEIGHBOR(up, i) & empty))
        retmask |= up;
    }
    
    to ^= next;
  }
  
  return retmask;
}


/**
 * Determines recursively the minimum of moves thorugh unthreatened fields
 * for a man needed to become a king.
 *
 * \param board Board configuration
 * \param pos The position of the brick to check
 * \param color The color of this brick (BLACK or WHITE)
 * 
 * \return The amount of moves for a man needed to become a king
 *          or -1 if there is no unthreatened path to the hostile back row.
 */

int runaway(bitboard *board, int pos, bool color)
{
  const bitboard empty = ~(board[WHITE] | board[BLACK]);

  /* In this function the variable next is *not* used for the current field */

  bitboard next,     /* Neighboring field */
           nextnext; /* Neighbor to a neighboring field into the same dir */
  int8 currway=-1,   /* Runaway analysis for the current field */
       minval=10;    /* Minimum runaway analysis of free neighboring fields */
  int i=0;

  /* Return 0 if hostile back row is reached */
  if (pos & king_bits[(int)color]) return 0;
  
  for (i = 0; i < N_DIRS; i++) { /* Check both directions (NW/SE & NE/SW) */
  
    /* Determine neighbors */
    if (color == BLACK) {
      next = DOWN_NEIGHBOR(pos, i);
      nextnext = DOWN_NEIGHBOR(DOWN_NEIGHBOR(pos, i), i);
    } else {
      next = UP_NEIGHBOR(pos, i);
      nextnext = UP_NEIGHBOR(UP_NEIGHBOR(pos, i), i);
    }

/*
printf("*** Field %d: threatened by enemy %d=%d\n", \
  fieldnumber(next),  T_ENEMY(T_COLOR(color)), threatened(board, T_ENEMY(T_COLOR(color))) & next);  
printf("*** Field %d\n", \
  fieldnumber(next));
*/
printf(""); /* Needed to work properly (Weired !!!) */ 

    /* Check recursively if field is empty and unthreatened */
    if (next & empty & ~(threatened(board, T_ENEMY(T_COLOR(color))) & next))
      currway = runaway(board, next, color);

    /* Determine minimum runaway value if there was a path
       to the hostile home row found                       */
    if ((currway > -1) && (currway < minval)) minval = currway;
  }
  
  /* Return minimum value + 1 */
  if (minval == 10) return -1; else return minval + 1;
}


/**
 * Determines recursively how many bricks a brick of a certain kind could kill
 * standing on a certain field.
 *
 * \param board Board configuration
 * \param color The color of the brick that is possibly a killer (BLACK or WHITE)
 * \param moves The directions this brick can move to (UP, DOWN or UP+DOWN)
 * \param pos The position on which on the brick is standing
 * \param from_v The vertical direction the brick is coming from (UP or DOWN)
 * \param from_h The horizontal direction the brick is coming from (0 or 1)
 * 
 * \return The number of enemies that can be killed in a row starting from pos
 */

int8 poss_kills(bitboard *board, bool color, int8 moves, bitboard pos,
               int8 from_v, int8 from_h)
{
  const bitboard empty = ~(board[WHITE] | board[BLACK]);
  
  /* In this function the variable next is *not* used for the current field */
  
  bitboard next,     /* Neighboring field */
           nextnext; /* Neighbor to a neighboring field into the same dir */
  bitboard newboard[N_BOARDS];
  int8 morekills=0,  /* Amount of further kills possible from a position */
       maxkills=0;   /* Maximum of possible kills from position pos */
  int i=0;
  
  for (i = 0; i < N_DIRS; i++) { /* Check both directions (NW/SE & NE/SW) */
    
    /* If the brick can move down and did not come from exact that direction */
    if ((moves & DOWN) && !((from_v & DOWN) && (from_h & i))) {
    
      /* Determine neighboring fields */
      next = DOWN_NEIGHBOR(pos, i);
      nextnext = DOWN_NEIGHBOR(DOWN_NEIGHBOR(pos, i), i);
      
      /* Call poss_kills recursively if neighbor can be killed */
      if ((next & board[ENEMY(color)]) && (nextnext & empty)) {
        newboard[WHITE] = board[WHITE] & ~next & ~pos;
        newboard[BLACK] = board[BLACK] & ~next & ~pos;
        newboard[KING] = board[KING] & ~next & ~pos;
        
        morekills = poss_kills(newboard, color, moves, nextnext, UP, i);
        
        /* The amount of possible kills into one direction is determined by
           further kills after the first on, or equals otherwise one if the
           target field is not threatened by an enemy                       */
        if (morekills > 0) {
          if (1 + morekills > maxkills) maxkills = 1 + morekills;
        } else {
          if (nextnext & ~threatened(newboard, T_ENEMY(T_COLOR(color))))
            if (1 > maxkills) maxkills = 1;
        }
      } 
    }
    
    /* If the brick can move up and did not come from exact that direction */
    if ((moves & UP) && !((from_v & UP) && (from_h & i))) {
      
      /* Determine neighboring fields */
      next = UP_NEIGHBOR(pos, i);
      nextnext = UP_NEIGHBOR(UP_NEIGHBOR(pos, i), i);
      
      /* Call poss_kills recursively if neighbor can be killed */
      if ((next & board[ENEMY(color)]) && (nextnext & empty)) {
        newboard[WHITE] = board[WHITE] & ~next & ~pos;
        newboard[BLACK] = board[BLACK] & ~next & ~pos;
        newboard[KING] = board[KING] & ~next & ~pos;
        
        morekills = poss_kills(newboard, color, moves, nextnext, DOWN, i);
        
        /* The amount of possible kills into one direction is determined by
           further kills after the first on, or equals otherwise one if the
           target field is not threatened by an enemy                       */
        if (morekills > 0) {
          if (1 + morekills > maxkills) maxkills = 1 + morekills;
        } else {
          if (nextnext & ~threatened(newboard, T_ENEMY(T_COLOR(color))))
            if (1 > maxkills) maxkills = 1;
        }
      } 
    }
  }
  
  return maxkills;
}

    
/**
 * Used for debugging output purposes only
 *
 * \return The number of the marked field in the mask, or
 *         -1 if there are several fields marked.
 */

int fieldnumber(bitboard mask)
{
  int8 retval=0, i=0;

  for (i=0; i<BOARD_SIZE; i++) {
    if (mask & 1) {
      if (retval != 0) return -1;
      retval += i;
    }
    mask >>= 1;
  }
  
  return retval+1;
}

  
