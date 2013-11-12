
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include "checkers.h"

/* global variables for search parameters */
static int n_evals, n_nodes, n_hash, top_depth;
static int time_allowed;

/* best move found so far */
static struct move *best_move_p;

/* transposition table */
static struct hash_pos *trans_table;

/* should be set in main.o or test.o */
extern jmp_buf env;
extern sigset_t alarm_set;

void alarm_handler(int signal)
{
  printf("Completed %d depths, %d nodes, %d evals, %d hash hits (%.02lf eval/sec)\n", 
	 top_depth - 1, n_nodes, n_evals, n_hash, (double)n_evals/time_allowed);

  longjmp(env, 1);
}

/* alpha beta function "with memory */
int alpha_beta(bitboard *board, int alpha, int beta, int depth, const bool color)
{
  int val, next_val, best_alpha, n_moves, i;
  int hash_key;
  struct move move_list[MAX_MOVES];
  struct move best_move;
  struct hash_pos *hash_entry;
  bool has_best_move = FALSE;

  n_nodes++;

#ifdef DEBUG 
  printf("SEARCH: depth = %d, alpha = %d, beta = %d\n", depth, alpha, beta);
  printf("SEARCH: %s to move\n", color ? "black" : "white");
  print_board(board);
#endif
  
  /* get the appropriate hash_pos */
  hash_key = HASH_KEY(board) % TRANS_TABLE_SIZE;
  hash_entry = &trans_table[hash_key];

  if (board[WHITE] != hash_entry->board[WHITE] ||
      board[BLACK] != hash_entry->board[BLACK] ||
      board[KING] != hash_entry->board[KING] ||
      color != hash_entry->color) {
    hash_entry = NULL;
  }

  if (hash_entry) {
    n_hash++;
#ifdef DEBUG
    printf("SEARCH: Position has hash entry\n");
    printf("SEARCH: Hash, high = %d (%d), low = %d (%d)\n", 
	   hash_entry->high_bound,
	   hash_entry->high_depth,
	   hash_entry->low_bound,
	   hash_entry->low_depth);
#endif
    if (hash_entry->low_depth >= depth) {
      if (hash_entry->low_bound >= beta) {
	if (depth == top_depth) {
#ifdef DEBUG
	  printf("SEARCH: Setting GLOBAL Best move to:\n");
	  print_board(best_move.board);
#endif
	  COPY_BOARD(best_move_p->board, hash_entry->best_move.board);
	}

	return hash_entry->low_bound;
      }

      if (hash_entry->low_bound > alpha)
	alpha = hash_entry->low_bound;
    }
    
    if (hash_entry->high_depth >= depth) {
      if (hash_entry->high_bound <= alpha) {
	if (depth == top_depth) {
#ifdef DEBUG
	  printf("SEARCH: Setting GLOBAL Best move to:\n");
	  print_board(best_move.board);
#endif
	  COPY_BOARD(best_move_p->board, hash_entry->best_move.board);
	}

	return hash_entry->high_bound;
      }

      if (hash_entry->high_bound > beta)
	beta = hash_entry->high_bound;
    }
  }


  if (depth == 0) {
    n_evals++;
    val = color ? eval(board, BLACK) : -eval(board, WHITE);
  }
  else {
    val = -INFINITY;
    best_alpha = alpha;

    /* try best move if one exists */
    if (hash_entry && hash_entry->has_best_move) {
#ifdef DEBUG 
      printf("SEARCH: Using best move from hash table\n");
#endif
      COPY_BOARD(best_move.board, hash_entry->best_move.board);
      has_best_move = TRUE;

      val = -alpha_beta(hash_entry->best_move.board, -beta, -alpha, depth - 1, !color);
      if (val > best_alpha) {
	best_alpha = val;
	if (depth == top_depth) {
	  sigprocmask(SIG_BLOCK, &alarm_set, NULL);
#ifdef DEBUG
	  printf("SEARCH: Setting GLOBAL Best move to:\n");
	  print_board(best_move.board);
#endif
	  COPY_BOARD(best_move_p->board, best_move.board);
	  sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);
	}
      }
    }

    n_moves = generate_moves(board, color, move_list);

    /* no moves? we lose! */
    /* what to do here 
    if (n_moves == 0)
      return -INFINITY;
    */
    
#ifdef DEBUG
    printf("SEARCH: Going into move search, val = %d, beta = %d\n", val, beta);
#endif
    for (i = 0; i < n_moves && val < beta; i++) {
      next_val = -alpha_beta(move_list[i].board, -beta, -best_alpha, depth - 1, !color);
      if (next_val > val) {
	val = next_val;
	COPY_BOARD(best_move.board, move_list[i].board);
	has_best_move = TRUE;
      }
      if (next_val > best_alpha) {
	best_alpha = next_val;
	if (depth == top_depth) {
	  sigprocmask(SIG_BLOCK, &alarm_set, NULL);
#ifdef DEBUG
	  printf("SEARCH: Setting GLOBAL Best move to:\n");
	  print_board(move_list[i].board);
#endif

	  COPY_BOARD(best_move_p->board, move_list[i].board);
	  sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);
	}
      }
    }
  }

  hash_entry = &trans_table[hash_key];

  sigprocmask(SIG_BLOCK, &alarm_set, NULL);
#ifdef DEBUG
  printf("Storing hash (%08x) at level %d (%s) for position:\n", 
	 hash_key, depth, color ? "black" : "white");
  print_board(board);
  if (has_best_move) {
    printf("Best move:\n");
    print_board(best_move.board);
  }
  printf("val = %d [%d, %d]\n", val, alpha, beta);
#endif

  hash_entry->color = color;
  hash_entry->has_best_move = has_best_move;
  COPY_BOARD(hash_entry->board, board);
  COPY_BOARD(hash_entry->best_move.board, best_move.board); 

  if (val <= alpha) {
    hash_entry->high_bound = val;
    hash_entry->high_depth = depth;
    hash_entry->low_bound = -INFINITY;
    hash_entry->low_depth = 0;
  }
  else if (val >= beta) {
    hash_entry->high_bound = INFINITY;
    hash_entry->high_depth = 0;
    hash_entry->low_bound = val;
    hash_entry->low_depth = depth;
  }
  else {
    hash_entry->high_bound = val;
    hash_entry->high_depth = depth;
    hash_entry->low_bound = val;
    hash_entry->low_depth = depth;
  }
  sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);
  
  return val;
}

void mtdf(bitboard *board, struct move *best_move, 
	  const bool color, unsigned int time_s)
{
  int i, val, beta, lower_bound, upper_bound;

  /* initialize transposition table, if necessary */
  if (trans_table == NULL) {
    trans_table = (struct hash_pos *)malloc(sizeof(struct hash_pos) * TRANS_TABLE_SIZE);
    memset(trans_table, 0, sizeof(struct hash_pos) * TRANS_TABLE_SIZE);
  }

  /* initialize global variables */
  n_evals = n_nodes = n_hash = 0;
  best_move_p = best_move;

  /* first iteration */
  top_depth = 1;
  val = alpha_beta(board, -INFINITY, INFINITY, 1, color);
#ifdef DEBUG
  printf("SEARCH: After first iteration, val = %d\n", val);
#endif

  /* start alarm */
  time_allowed = time_s;
  signal(SIGALRM, alarm_handler);
  alarm(time_s);

  /* continue iterating using iterative deepening */
  for (i = 2; ;i++) {
    if (val >= INFINITY || val <= -INFINITY)
      break;

    top_depth = i;
#ifdef DEBUG
    printf("SEARCH: Searching to depth %d\n", i);
#endif
    
    upper_bound = INFINITY;
    lower_bound = -INFINITY;

    while (upper_bound > lower_bound) {
      if (val == lower_bound)
	beta = val + 1;
      else 
	beta = val;

      val = alpha_beta(board, beta - 1, beta, i, color);
#ifdef DEBUG
      printf("SEARCH: alpha_beta return, val = %d, beta = %d, [%d, %d]\n", 
	     val, beta, lower_bound, upper_bound);
#endif
      
      if (val < beta)
	upper_bound = val;
      else 
	lower_bound = val;
    }

#ifdef DEBUG
    printf("SEARCH: After %dth iteration, val = %d\n", i, val);
    printf("SEARCH: Best move\n");
    print_board(best_move->board);
#endif
  }
  alarm(0);

  printf("Completed %d depths, %d nodes, %d evals, %d hash hits (%.02lf eval/sec)\n", 
	 top_depth - 1, n_nodes, n_evals, n_hash, (double)n_evals/time_allowed);
}
