OUT_PATH := ../../../build/backend/eeyore
UTILS_PATH := ../../utils
SYNTAX_ANALYSIS_PATH := ../../frontend/syntax_analysis

CC := g++
CC_FLAGS := -c -std=c++17 -I $(UTILS_PATH) -I $(SYNTAX_ANALYSIS_PATH)

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))

mkdir:
	mkdir -p $(OUT_PATH)

$(OBJS): %.o: %.cc mkdir
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all: $(OBJS)

clean:
	rm -r $(OUT_PATH)/*.o
