#include <stdio.h>
#include <string.h>
#include "checkers.h"



/* function prototypes */
bitboard possible_moves(bitboard *board); // Returns possible moves for a field
void print_numbers();        // Prints out the field numbers for orientation


int main(int argc, char *argv[])
{
  printf("here we go...\n");

  int val;
  bitboard b[N_BOARDS]; 
  char *filename = "starts/testeval.wdp";
  bool color=WHITE;
  bool *colorp;
  colorp = &color;
 
  if (argc>1)
    read_wdp(b, argv[1], colorp);
  else 
    read_wdp(b, filename, colorp);
 
val = eval(b, color);
  printf("\nTotal evaluation: %d\n\n", val);

  printf("\nCurrent board:\n");
  print_board(b);
  
  printf("\nThreatened by white:\n");
  bitboard x[N_BOARDS];
  x[WHITE] = threatened(b, T_WHITE);
  x[BLACK] = 0;
  x[KING] = 0;
  print_board(x);
  
  printf("\nThreatened by black:\n");
  x[WHITE] = 0;
  x[BLACK] = threatened(b, T_BLACK);
  print_board(x);
  
  printf("\nFields to move to:\n");
  x[WHITE] = possible_moves(b);
  x[BLACK] = 0;
  print_board(x);
}


/*
 * possible_moves
 *
 * Computes a mask with those fields marked which existing bricks can move to
 *
 * Needs:   Board configuration *board
 * 
 * Returns: The mask
 */

bitboard possible_moves(bitboard *board)
{
  const bitboard empty = ~(board[WHITE] | board[BLACK]);
  
  bitboard to = board[BLACK]|board[WHITE];
  bitboard retmask=0; /* Mask to return */
  bitboard next=0,    /* Mask to the current brick */
           down=0,    /* Mask to down neighbor */
           up=0;      /* Mask to up neighbor */
  int i=0;
  
  while (to) {
    next = LAST_ONE(to);
    
    for (i = 0; i < N_DIRS; i++) { /* Check both directions (NW/SE & NE/SW) */
    
      /* Determine neighbors; down and up will be zero if the current brick
         cannot move into this direction                                    */
      down = DOWN_NEIGHBOR((next&board[BLACK]) |
                           (next&board[WHITE]&board[KING]), i);
      up = UP_NEIGHBOR((next&board[WHITE]) |
                       (next&board[BLACK]&board[KING]), i);
      
      
      /* Add down neighbor if the brick can move downwards and the field is
         either empty or occupied by an enemy and the field behind is empty. */
      if (down & empty) {
        retmask |= down;
      } else {
        if (((next&board[BLACK] && down&board[WHITE]) ||
             (next&board[WHITE] && down&board[BLACK])) &&
            (DOWN_NEIGHBOR(down, i) & empty))
          retmask |= DOWN_NEIGHBOR(down, i);
      }
      
      /* Add up neighbor if the brick can move upwards and the field is
         either empty or occupied by an enemy and the field behind is empty. */
      if (up & empty) {
        retmask |= up;
      } else {
        if (((next&board[BLACK] && up&board[WHITE]) ||
             (next&board[WHITE] && up&board[BLACK])) &&
            (UP_NEIGHBOR(up, i) & empty))
          retmask |= UP_NEIGHBOR(up, i);
      }
    }
    
    to ^= next;
  }
  
  return retmask;
}


/*
 * print_numbers
 *
 * Prints out a numbered checkers board for orientation
 */

void print_numbers()
{
  int i=0;
  
  for (i=0; i<BOARD_SIZE; i++) {
    if (((i)%4 == 0) && ((i)/4)%2 == 0) printf("  ");
    printf("%2d", i+1);
    printf("  ");
    if (i%4 == 3) printf("\n");
  }
}
