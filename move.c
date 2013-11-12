#ifdef DEBUG
#include <stdio.h>
#endif

#include <string.h>
#include "checkers.h"

int generate_moves(bitboard *board, const bool color, struct move *move_list)
{
  int n = 0;

#ifdef DEBUG  
  printf("MOVE: generate_moves, color = %s\n", color ? "black" : "white");
#endif 
 
  if ((n = try_capture(board, 0xffffffff, color, move_list)) > 0) {    
    return n;
  }

  return try_move(board, color, move_list);
}

int try_move(bitboard *board, const bool color, struct move *next_move)
{
  bitboard next, to, from;
  const bitboard empty = ~(board[BLACK] | board[WHITE]);
  const bitboard move_up = color ? board[KING] : 0xffffffff;
  const bitboard move_down = !color ? board[KING] : 0xffffffff;
    
  int n = 0, i;

#ifdef DEBUG
  printf("MOVE: try_move\n");
#endif

  // check down neighbors
  for (i = 0; i < N_DIRS; i++) {
    to = DOWN_NEIGHBOR(board[(int)color] & move_down, i) & empty; 
    while (to) {
      next = LAST_ONE(to);
      from = UP_NEIGHBOR(next, i);

      // modify move board
      next_move->board[(int)color] = (board[(int)color] | next) & ~from;
      next_move->board[(int)!color] = board[(int)!color];
      next_move->board[KING] = ((board[KING] | (next & king_bits[(int)color])) & ~from) | 
	(DOWN_NEIGHBOR(board[KING], i) & next);

      next_move++;
      n++;
      to ^= next;
    }
  }

  // check up neighbors
  for (i = 0; i < N_DIRS; i++) {
    to = UP_NEIGHBOR(board[(int)color] & move_up, i) & empty;
    while (to) {
      next = LAST_ONE(to);
      from = DOWN_NEIGHBOR(next, i);

      // modify move board
      next_move->board[(int)color] = (board[(int)color] | next) & ~from;
      next_move->board[(int)!color] = board[(int)!color];
      next_move->board[KING] = ((board[KING] | (next & king_bits[(int)color])) & ~from) | 
	(UP_NEIGHBOR(board[KING], i) & next);

      next_move++;
      n++;
      to ^= next;
    }
  }
  
#ifdef DEBUG
  printf("MOVE: %d possible moves\n", n);
#endif
  
  return n;
}

int try_capture(bitboard *board, bitboard mask, const bool color, struct move *next_move)
{
  bitboard to, cap, from, next;
  const bitboard empty = ~(board[WHITE] | board[BLACK]);
  const bitboard move_up = (color ? board[KING] : 0xffffffff) & mask;
  const bitboard move_down = (!color ? board[KING] : 0xffffffff) & mask;
  struct move cur_move;

  int n = 0, i, leaves = 0;

#ifdef DEBUG
  printf("MOVE: try_capture\n");
#endif

  // check down neighbors
  for (i = 0; i < N_DIRS; i++) {
    to = DOWN_NEIGHBOR(DOWN_NEIGHBOR(board[(int)color] & move_down, i) & board[(int)!color], i) & empty;
    while (to) {
      next = LAST_ONE(to);
      cap = UP_NEIGHBOR(next, i);
      from = UP_NEIGHBOR(cap, i);

      to ^= next;
      
      cur_move.board[(int)color] = (board[(int)color] & ~from) | next;
      cur_move.board[(int)!color] = board[(int)!color] & ~cap;
      cur_move.board[KING] = ((board[KING] & ~from) & ~cap) | 
	(next & (king_bits[(int)color] | DOWN_NEIGHBOR(DOWN_NEIGHBOR(board[KING], i), i)));

      /* must stop once we get a king */
      if (!(from & board[KING]) && (next & cur_move.board[KING]))
	next = 0;
      
      if ((n = try_capture(cur_move.board, next, color, next_move+leaves)) == 0) {
	(next_move+leaves)->board[WHITE] = cur_move.board[WHITE];
	(next_move+leaves)->board[BLACK] = cur_move.board[BLACK];
	(next_move+leaves)->board[KING] = cur_move.board[KING];
	leaves++;
      }
      else
	leaves += n;

    }
  }
  
  // check up neighbors
  for (i = 0; i < N_DIRS; i++) {
    to = UP_NEIGHBOR(UP_NEIGHBOR(board[(int)color] & move_up, i) & board[(int)!color], i) & empty;
    while (to) {
      next = LAST_ONE(to);
      cap = DOWN_NEIGHBOR(next, i);
      from = DOWN_NEIGHBOR(cap, i);

      to ^= next;

      cur_move.board[(int)color] = (board[(int)color] & ~from) | next;
      cur_move.board[(int)!color] = board[(int)!color] & ~cap;
      cur_move.board[KING] = ((board[KING] & ~from) & ~cap) | 
	(next & (king_bits[(int)color] | UP_NEIGHBOR(UP_NEIGHBOR(board[KING], i), i)));

      /* must stop once we get a king */
      if (!(from & board[KING]) && (next & cur_move.board[KING]))
	next = 0;
      
      if ((n = try_capture(cur_move.board, next, color, next_move+leaves)) == 0) {
	(next_move+leaves)->board[WHITE] = cur_move.board[WHITE];
	(next_move+leaves)->board[BLACK] = cur_move.board[BLACK];
	(next_move+leaves)->board[KING] = cur_move.board[KING];
	leaves++;
      }
      else
	leaves += n;
    }
  }
  
#ifdef DEBUG
  printf("MOVE: leaves in capture tree = %d\n", leaves);
#endif
  
  return leaves;
}



