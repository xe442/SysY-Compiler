MAKE = make
CC = g++

#### normal build

all:
	cd ./src; make all
	cd ./build; find ./ -name "*.o" | xargs g++ -o compiler
clean:
	rm -rf ./build


#### TODO: platform build
# Copying all the files to a single directory, namely ./sim/, and build them all.

# Copys all the ".h" and ".cc" files from ./src/* and to ./sim/
sim_copy:
	find ./src -type f -regex ".*\.\(h\|cc\)" -exec cp {} ./sim \;
