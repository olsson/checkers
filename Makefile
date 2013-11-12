# Makefile for checkers program

CC=gcc
#CC_FLAGS=-Wall -DDEBUG -g
#CC_FLAGS=-Wall -g
# Don't use, breaks UI code!
#CC_FLAGS=-O2
TIMESTAMP=`/usr/bin/date +%y%m%d%H%M`
CP=/usr/bin/cp
REF_DIR=ref

OBJS=move.o io.o eval.o search.o
INC=checkers.h
CHECKERS=checkers
CHECKERS_OBJ=main.o
TEST=test
TEST_OBJ=test.o

all: $(CHECKERS) $(TEST)

ref: $(CHECKERS)
	$(CP) $(CHECKERS) $(REF_DIR)/$(CHECKERS)_$(TIMESTAMP)

$(CHECKERS): $(OBJS) $(CHECKERS_OBJ)
	$(CC) $(CC_FLAGS) -o $(CHECKERS) $(OBJS) $(CHECKERS_OBJ)

$(TEST): $(OBJS) $(TEST_OBJ)
	$(CC) $(CC_FLAGS) -o $(TEST) $(OBJS) $(TEST_OBJ)

$(OBJS) $(CHECKERS_OBJ) $(TEST_OBJ): $(INC) Makefile

.c.o: 
	$(CC) $(CC_FLAGS) -c $(<)

clean:
	rm -f $(OBJS) $(CHECKERS) $(CHECKERS_OBJ) $(TEST) $(TEST_OBJ) *~ starts/*~ *exe





