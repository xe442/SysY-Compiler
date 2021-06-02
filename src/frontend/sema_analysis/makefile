OUT_PATH := ../../../build/frontend/sema_analysis
UTILS_PATH := ../../utils
SYNTAX_ANALYSIS_PATH := ../syntax_analysis

CC := g++ -I $(UTILS_PATH) -I $(SYNTAX_ANALYSIS_PATH)
CC_FLAGS := -c -std=c++17

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))

mkdir:
	mkdir -p $(OUT_PATH)

$(OBJS): %.o: %.cc mkdir
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all: $(OBJS)

clean:
	rm -r $(OUT_PATH)/*.o
