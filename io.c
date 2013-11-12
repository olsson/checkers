#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checkers.h"

int read_wdp(bitboard *board, const char *filename, bool *color, double *time)
{
  int n = 0, i;
  char c;
  FILE *fd;
  
  for (i = 0; i < N_BOARDS; i++)
    board[i] = 0;

  fd = fopen(filename, "r");
  if (fd == NULL) {
    perror(filename);
    return 0;
  }

  while (((c = fgetc(fd)) != EOF) && (n < 32)) {
    switch (c) {
    case 'B':
      board[KING] |= 1<<n;
    case 'b':
      board[BLACK] |= 1<<n;
      n++;
      break;
    case 'W':
      board[KING] |= 1<<n;
    case 'w':
      board[WHITE] |= 1<<n;
      n++;
      break;
    case '.':
      n++;
    }
  }

  n = 0;
  while (((c = fgetc(fd)) != EOF) && (n < 1)) {
    switch(c) {
    case 'b':
      *color = BLACK;
      n++;
      break;
    case 'w':
      *color = WHITE;
      n++;
    }
  }
  
  if (!fscanf(fd, "%lf", time)) 
    *time = 300;
  
  fclose(fd);

  return (n == 1);
}

void print_board(bitboard *board)
{
  int i, before = 1, mask, n_rows = 0;;

  printf(" _______________ \t%s\n", board_pos_str[n_rows++]);


  for (i = 0; i < BOARD_SIZE; i++) {
    mask = 1<<i;
    
    if (i%4 == 0) {
      printf("|");
    }
    if (before) printf("  ");

    printf("%c", board[BLACK]&mask ? (board[KING]&mask ? 'B' : 'b') :
	        (board[WHITE]&mask ? (board[KING]&mask ? 'W' : 'w') : ' ')); 

    if (!before) printf("  ");

    if ((i+1)%4 == 0) {
      printf("|\t%s\n", board_pos_str[n_rows++]);
      before = 1 - before;
    }
    else
      printf(" ");
  }

  printf(" --------------- \t%s\n", board_pos_str[n_rows++]);
}

void print_move_list(bitboard *board, bool color, struct move *move_list, int n)
{
  char buf[128];
  int i;

  for (i = 0; i < n; i++) {
    printf("move option #%d\n", i+1);
    print_board(move_list[i].board);
    trans_move_string(board, &move_list[i], buf, color);
    printf("%s\n", buf);
  }
}

bool trans_string_move(bitboard *board, struct move *move, 
		       char *move_str, bool color)
{
  int from, to;
  bitboard from_board, to_board, jump_board;
  bitboard empty, move_up, move_down;
  int high_row, low_row;
  int dir;
  char *p;

  move->board[WHITE] = board[WHITE];
  move->board[BLACK] = board[BLACK];
  move->board[KING] = board[KING];

  p = move_str;
  while (p && sscanf(p, "%d-%d", &from, &to) == 2) {
    empty = ~(move->board[BLACK] | move->board[WHITE]);
    move_up = color ? move->board[KING] : 0xffffffff;
    move_down = !color ? move->board[KING] : 0xffffffff;
    
    from--;
    to--;

    if (from > to) {
      high_row = from / 4;
      low_row = to / 4;
      dir = UP;
    }
    else {
      high_row = to / 4;
      low_row = from / 4;
      dir = DOWN;
    }

    from_board = 1<<from;
    to_board = 1<<to;
    jump_board = 0;

    if (high_row - low_row == 2) {
      if (dir == UP) {
        if (to % 4 > from % 4)
          jump_board = UP_NEIGHBOR(from_board, NE);
        else 
          jump_board = UP_NEIGHBOR(from_board, NW);
       } else {
         if (to % 4 > from % 4)
           jump_board = DOWN_NEIGHBOR(from_board, SE);
         else 
           jump_board = DOWN_NEIGHBOR(from_board, SW);
       }
      }
      else if (high_row - low_row != 1) 
        return FALSE;

      /* check that from pos has our piece */
      if (!(move->board[(int)color] & from_board)) 
        return FALSE;
      
      /* check that to pos is empty */
      if (!(empty & to_board))
        return FALSE;
      
      /* check that jump pos (if exists) has opposite color */
      if (jump_board && !(move->board[(int)!color] & jump_board))
        return FALSE;
  
      /* check that this piece can move in this direction */
      if (dir == UP) {
        if (!(move->board[(int)color] & move_up))
          return FALSE;
      }
      else {
        if (!(move->board[(int)color] & move_down))
          return FALSE;
      }

      move->board[(int)color] |= to_board;
      move->board[(int)color] &= ~from_board;
      move->board[(int)!color] &= ~jump_board;
      move->board[KING] &= ~jump_board;
      if (move->board[KING] & from_board) {
        move->board[KING] |= to_board;
        move->board[KING] &= ~from_board;
      }
      else 
        move->board[KING] |= king_bits[(int)color] & to_board;
    
      p = strchr(p, '-') + 1;
   } 

   return TRUE;
}

void trans_move_string(bitboard *board, struct move *move, 
		       char *move_str, bool color)
{
  bitboard pos[10];          /* from/to and intermediate positions */
  int n_pos = 1, i, n;
  bitboard from, to, diff;

  from = (board[(int)color] ^ move->board[(int)color]) & board[(int)color];
  to = (board[(int)color] ^ move->board[(int)color]) & move->board[(int)color];
  
  diff = board[!color] ^ move->board[!color];

  pos[0] = from;
  if (diff)
    n_pos += trans_move_string_recur(diff, pos + 1) - 2;
  pos[n_pos++] = to;
       		    
  for (i = 0; i < n_pos; i++) {
    n = 0;
    while (pos[i]) {
      n++;
      pos[i] = pos[i] >> 1;
    }

    move_str += ((i == 0) ? sprintf(move_str, "%d", n) : sprintf(move_str, "-%d", n));
  }
}

int trans_move_string_recur(bitboard diff, bitboard *pos)
{
  int i, n;

  if (!diff)
    return 1;
  
  // check down neighbors
  for (i = 0; i < N_DIRS; i++)
    if (DOWN_NEIGHBOR(*(pos - 1), i) & diff) {
      *pos = DOWN_NEIGHBOR(DOWN_NEIGHBOR(*(pos - 1), i), i);
      if ((n = trans_move_string_recur(diff ^ DOWN_NEIGHBOR(*(pos - 1), i), pos + 1)))
	return n + 1;
    }

  for (i = 0; i < N_DIRS; i++)
    if (UP_NEIGHBOR(*(pos - 1), i) & diff) {
      *pos = UP_NEIGHBOR(UP_NEIGHBOR(*(pos - 1), i), i);
      if ((n = trans_move_string_recur(diff ^ UP_NEIGHBOR(*(pos - 1), i), pos + 1)))
	return n + 1;
    }

  return 0;
}

int valid_move_input(const char *input) {

  int c = 0;
  char l = input[c];

  while (1) {
    if ((l <= '9' && l >= '0') || l == '-') {
      c++;
      l = input[c];
      if (l == 0)
	break;
    }
    else {

#ifdef DEBUG
      printf("Scanning error! Encountered: %c\n", l);
#endif

      return 0;
    }
  }
  
  return 1;

}



/** 
 * Outputs the current board state, along with the number of moves
 * specified from move_list, and the time left, into filename.
 */
int write_log(const bitboard *board, char move_list[][128], 
	      const char *filename, int moves, bool *color, double time_left)
{

  int i, before = 1, mask;
  FILE *fd;

  fd = fopen(filename, "w");
  if (fd == NULL) {
    perror(filename);
    return 0;
  }

  for (i = 0; i < BOARD_SIZE; i++) {
    mask = 1<<i;
        
    fprintf(fd, "%c", board[BLACK]&mask ? (board[KING]&mask ? 'B' : 'b') :
	    (board[WHITE]&mask ? (board[KING]&mask ? 'W' : 'w') : '.'));

    if (!before) fprintf(fd, " ");

    if ((i+1)%4 == 0)
      fprintf(fd, " ");

  }

  fprintf(fd, " %s ", (color ? "b" : "w"));

  fprintf(fd, "%.2lf\n", time_left);
  
  for (i = 0; i < moves; i++) {
    fprintf(fd, "%d. %s\n", i+1, move_list[i]);
  }



  fclose(fd);
  return 1;

}
