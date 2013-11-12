/* test suite for checkers program */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <signal.h>
#include "checkers.h"

jmp_buf env;
sigset_t alarm_set;

void test_neighbor() 
{
  int i, j;

  bitboard board[N_BOARDS];

  printf("Testing neighbor location ... \n");
  printf("  starting position shown as white, neighbors are black\n");
  board[BLACK] = 0;
  board[KING] = 0;
  for (i = 0; i < BOARD_SIZE; i++) {
    board[WHITE] = 1<<i;
    board[BLACK] = 0;
    
    for (j = 0; j < N_DIRS; j++)
      board[BLACK] |= DOWN_NEIGHBOR(board[WHITE], j);
    for (j = 0; j < N_DIRS; j++)
      board[BLACK] |= UP_NEIGHBOR(board[WHITE], j);
    
    print_board(board);
  }
}

void test_move(char *start_file) {
  int n;
  struct move move_list[MAX_MOVES];
  bitboard board[N_BOARDS];
  bool color;

  printf("Testing Move List Generation ... \n");

  memset(move_list, 0, MAX_MOVES * sizeof(struct move));
  if (!read_wdp(board, start_file, &color))
    printf("error reading %s\n", start_file);
  else {
    printf("Starting position:\n");
    print_board(board);
    n = generate_moves(board, color, move_list);
    printf("Possible moves:\n");
    print_move_list(board, color, move_list, n);
  }
}

void test_search(char *start_file, int secs) {
  struct move best_move;
  bitboard board[N_BOARDS];
  char string[128];
  bool color;

  printf("Testing Search, with time = %d secs... \n", secs);
  if (!read_wdp(board, start_file, &color))
    printf("error reading %s\n", start_file);
  else {
    printf("Starting position:\n");
    print_board(board);

    /* set up handling of alarm signal */
    signal(SIGVTALRM, alarm_handler);
    sigemptyset(&alarm_set);
    sigaddset(&alarm_set, SIGALRM);

    clock();

    if (setjmp(env) == 0)
      mtdf(board, &best_move, color, secs);

    printf("Best move, time = %.2lf\n", (double)clock() / CLOCKS_PER_SEC);
    print_board(best_move.board);
    trans_move_string(board, &best_move, string, color);
    printf("%s\n", string);
  }
}

void test_trans(char *start_file, char *move)
{
  bool color;
  bitboard board[N_BOARDS];
  struct move opp_move;

  if (!read_wdp(board, start_file, &color)) 
    printf("error reading %s\n", start_file);
  else {
    printf("Starting position:\n");
    print_board(board);
    
    if (!trans_string_move(board, &opp_move, move, color)) {
      printf("invalid move\n");
      return;
    }

    print_board(opp_move.board);
  }
}

void test_eval(char *start_file)
{
  bool color;
  bitboard board[N_BOARDS];

  if (!read_wdp(board, start_file, &color)) 
    printf("error reading %s\n", start_file);
  else {
    printf("Starting position:\n");
    print_board(board);
    
    printf("Score = %d\n", eval(board, color));
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 0;

  if (!strcmp(argv[1], "-neighbor"))
    test_neighbor();

  if (argc < 3)
    return 0;

  else if (!strcmp(argv[1], "-move"))
    test_move(argv[2]);
  else if (!strcmp(argv[1], "-eval"))
    test_eval(argv[2]);

  if (argc < 4) 
    return 0;
  else if (!strcmp(argv[1], "-search"))
    test_search(argv[2], atoi(argv[3]));
  else if (!strcmp(argv[1], "-trans"))
    test_trans(argv[2], argv[3]);

  return 0;
}

