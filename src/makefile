OUT_PATH := ../build
UTILS_PATH := ./utils
DEFINE_PATH := ./define
FRONTEND_PATH := ./frontend
BACKEND_EEYORE_PATH := ./backend/eeyore
BACKEND_TIGGER_RISCV_PATH := ./backend/tigger_riscv

CC := g++
CC_FLAGS := -c -lfl -std=c++17 -I $(UTILS_PATH) -I $(DEFINE_PATH) -I $(FRONTEND_PATH) -I $(BACKEND_EEYORE_PATH) -I $(BACKEND_TIGGER_RISCV_PATH)

main.o: main.cc
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all:
	cd ./frontend/syntax_analysis; make lexer_parser
	cd ./utils; make all
	cd ./frontend/syntax_analysis; make all
	cd ./frontend/sema_analysis; make all
	cd ./backend/eeyore; make all
	make main.o

clean:
	cd ./frontend/syntax_analysis; make lexer_parser_clean
	rm -rf ../build/*
