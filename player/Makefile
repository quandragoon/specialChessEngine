CXX = gcc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c move_gen_reference.c search.c eval.c abort.c speculative_add.c
OBJ := $(addsuffix .o, $(basename $(SRC)))

ifeq ($(PARALLEL),1)
	PFLAG := -DPARALLEL -D_BSD_SOURCE # -g needed for test framework assertions
else
	PFLAG := 
endif

ifeq ($(TEST),1)
	CFLAGS := -Wall -O0 -g $(PFLAG) -fcilkplus # -g needed for test framework assertions
else
	CFLAGS := -Wall -O3 -DNDEBUG $(PFLAG) -fcilkplus
endif

LDFLAGS= -Wall -lrt -lm -lcilkrts -ldl -lpthread

%.o : %.c
	$(CXX) -std=c99 -c $(CFLAGS) $(LDFLAGS) $< -o $@

leiserchess : $(OBJ) leiserchess.o
	$(CXX) $(OBJ) leiserchess.o $(LDFLAGS) -o $@

time : clean leiserchess
	bash -c "LATEST=\$$(cat ../tests/compare.txt | perl -n -e'/player\/binaries\/(build-\d+)/ && print \$$1'); time ./leiserchess < tester.txt; time binaries/\$${LATEST} < tester.txt" 

compare : clean leiserchess
	bash -c "for i in {1..3}; do rm -f ../tests/compare.pgn; java -jar ../autotester/lauto.jar ../tests/compare.txt; ../tests/pgnrate.tcl ../tests/compare.pgn; done"

publish : clean leiserchess
	bash -c "STAMP=\$$(date +%s); cp leiserchess binaries/build-\$${STAMP}; sed -i 's/^\(invoke.*\/binaries\/\).*$$/\1build-'\$${STAMP}'/' ../tests/compare.txt; git add binaries/build-\$${STAMP}; git add ../tests/compare.txt"

clean :
	rm -f *.o *~ $(TARGET)
