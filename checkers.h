#ifndef _CHECKERS_H
#define _CHECKERS_H

typedef char bool;
typedef char int8;

/** 
 * Bitboard representation
 * 
 * 3 32-bit values are needed to represent the complete state of the 
 * game.  For convenience the colors are also boolean opposites so 
 * that we can easily refer to the opposite color and the board that
 * represents its positions.
 */
typedef unsigned int bitboard;


#define FALSE 0 
#define TRUE 1  

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) > (y)) ? (y) : (x))


static const char *const board_pos_str[] = 
{ " ________________ ",
  "|   1   2   3   4|",
  "| 5   6   7   8  |",
  "|   9  10  11  12|",
  "|13  14  15  16  |",
  "|  17  18  19  20|",
  "|21  22  23  24  |",
  "|  25  26  27  28|",
  "|29  30  31  32  |",
  " ---------------- " }; /*!< Board positions (purely for UI) */


#define WHITE FALSE
#define BLACK TRUE
#define KING 2
#define N_BOARDS 3
#define BOARD_SIZE 32

#define COPY_BOARD(x, y) \
x[BLACK] = y[BLACK]; \
x[WHITE] = y[WHITE]; \
x[KING] = y[KING];

#define ENEMY(x) (TRUE - (x))

/** Additional color representation in order to allow the combination of both */
#define T_WHITE (WHITE+1)
#define T_BLACK (BLACK+1)
#define T_BOTH (T_WHITE+T_BLACK)
#define T_ENEMY(x) (T_BOTH - (x))
#define T_COLOR(x) ((x) + 1)


/**
 * Bitmasks representing the king area in the board -- 
 * king_bits[WHITE] represents blacks back row
 * king_bits[BLACK] represents whites back row 
 */
static const bitboard king_bits[2] = { 0x0000000f, 0xf0000000 };

/**
 * Gets the last one (in a binary sense) from the number.  Used
 * to get a piece from one of the bitboard representations.
 */
#define LAST_ONE(x) ((x) & -(x))

/**
 * Generate bitmasks that represent moving a piece down left/right or 
 * up left/right.  Done it such a way so that DOWN_NEIGHBOR(x, i) and
 * UP_NEIGHBOR(x, i) represent opposite moves.
 * 
 * Would be nice to incorporate these into one macro but it doesn't 
 * seem possible since the different shift directions.  Also, non-king 
 * pieces can only move one way anyways, so its not so bad.
 */
#define DOWN_NEIGHBOR(x, i) \
(((((x) & 0xf0f0f0f0) << down_shift[(i)][0]) & 0x0f0f0f0f) | \
 ((((x) & 0x0f0f0f0f) << down_shift[(i)][1]) & 0xf0f0f0f0))
#define UP_NEIGHBOR(x, i) \
(((((x) & 0xf0f0f0f0) >> up_shift[(i)][0]) & 0x0f0f0f0f) | \
 ((((x) & 0x0f0f0f0f) >> up_shift[(i)][1]) & 0xf0f0f0f0))

/** Number of up/down dirs (two, left and right) */
#define N_DIRS 2 

static const int up_shift[N_DIRS][2] = 
{ { 4,  3},             /* NW */
  { 5,  4} };           /* NE */
static const int down_shift[N_DIRS][2] = 
{ { 3,  4},           /* SE */
  { 4,  5} };         /* SW */
  
#define SW 0
#define SE 1
#define NE 0
#define NW 1

#define UP 1
#define DOWN 2


/**
 * Represent a move with the resulting game board from this move. 
 * Because we use bitboard, this is perhaps the smallest represention
 * possible! 
 */
struct move {
  bitboard board[N_BOARDS];
};
#define MAX_MOVES 48


/** 
 * Hash keys for the transposition table 
 */
struct hash_pos {
  bitboard board[N_BOARDS];
  bool color;
  bool has_best_move;
  int high_bound, low_bound;
  unsigned char high_depth, low_depth;
  struct move best_move; 
};
#define BLACK_HASH 0xdeadbeef
#define HASH_KEY(board) (INT_HASH(board[WHITE]) ^ INT_HASH(board[BLACK]))

/** http://www.concentric.net/~Ttwang/tech/inthash.htm */
#define INT_HASH(key) ((((((key + ~(key << 15)) ^ (key >> 10)) + (key << 3)) ^ \
                       (key >> 6)) + ~(key << 11)) ^ (key >> 16))

/** 20mb (roughly) transposition table */
#define TRANS_TABLE_SIZE 500000 


/*
 * Multipliers for eval function
 */

#define MAT_MAN 100
#define MAT_KING 150
#define MAT_VICTORY 1000000
#define TRAPPED_KING 50
#define DOG_HOLE 10
#define SAFE_MAN 1
#define BLACK_TO_MOVE 3
#define RUNAWAY_MAN 50
#define RUNAWAY_ROW 7
#define BACK_3 4
#define BACK_2 12
#define BACK_1 18
#define BACK_0 20
#define POSS_KILL 30
#define POSS_ALTKILL 2
#define POSS_EXTRAKILL 80
#define KILL_FACT_TURN 1
#define KILL_FACT_NOTURN 0.5

#define INFINITY 2000000


       
/* function prototypes */

/* io.c */
int read_wdp(bitboard *board, const char *filename, bool *color, double *time);
void print_board(bitboard *board);
void print_move_list(bitboard *board, bool color, struct move *move_list, int n);
bool trans_string_move(bitboard *board, struct move *move, 
		       char *move_str, bool color);
void trans_move_string(bitboard *board, struct move *move, 
		       char *move_str, bool color);
int trans_move_string_recur(bitboard diff, bitboard *pos);
int valid_move_input(const char *input);
int write_log(const bitboard *board, char move_list[][128], 
	      const char *filename, int moves, bool *color, double time_left);

/* move.c */
int generate_moves(bitboard *board, const bool color, struct move *move_list);
int try_move(bitboard *board, const bool color, struct move *next_move);
int try_capture(bitboard *board, bitboard mask, const bool color, struct move *next_move);

/* search.c */

int nega_max(bitboard *board, int alpha, int beta, int depth, const bool color); 
void mtdf(bitboard *board, struct move *best_move, 
	  const bool color, unsigned int time_s);
void alarm_handler(int signal);

/* eval.c */
int eval(bitboard *board, bool btm);
bitboard threatened(bitboard *board, int8 t_color);
int fieldnumber(bitboard mask);
int runaway(bitboard *board, int pos, bool color);
int8 poss_kills(bitboard *board, bool color, int8 moves, bitboard pos,
                int8 from_v, int8 from_h);


#endif /* _CHECKERS_H */

