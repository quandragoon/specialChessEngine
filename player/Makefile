CXX = gcc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c move_gen_reference.c search.c eval.c abort.c speculative_add.c
OBJ := $(addsuffix .o, $(basename $(SRC)))

ifeq ($(PARALLEL),1)
	PFLAG := -DPARALLEL # -g needed for test framework assertions
else
	PFLAG := 
endif

ifeq ($(TEST),1)
	CFLAGS := -Wall -O0 -g -DTEST $(PFLAG) -fcilkplus # -g needed for test framework assertions
else
	CFLAGS := -Wall -O3 -DNDEBUG $(PFLAG) -fcilkplus
endif

LDFLAGS= -Wall -lrt -lm -lcilkrts -ldl

%.o : %.c
	$(CXX) -std=c99 -c $(CFLAGS) $(LDFLAGS) $< -o $@

leiserchess : $(OBJ) leiserchess.o
	$(CXX) $(OBJ) leiserchess.o $(LDFLAGS) -o $@

compare : clean leiserchess
	bash -c "for i in {1..3}; do rm -f ../tests/compare.pgn; java -jar ../autotester/lauto.jar ../tests/compare.txt; ../tests/pgnrate.tcl ../tests/compare.pgn; done"

publish : clean leiserchess
	bash -c "STAMP=\$$(date +%s); cp leiserchess binaries/build-\$${STAMP}; sed -i 's/^\(invoke.*\/binaries\/\).*$$/\1build-'\$${STAMP}'/' ../tests/compare.txt"

clean :
	rm -f *.o *~ $(TARGET)
