m :=Automated Makefile push message
CC := g++
bin := bin/
out := $(bin)ckript
flags := -O0 -g
src := src/
b := build/

objs := $(shell find $(src) -name '*.cpp' | sed -e 's/.cpp/.o/g' | sed -e 's/src\//build\//g')

all: $(out)

build/%.o: src/%.cpp src/%.hpp
	$(CC) $(flags) -c $< -o $@

$(out): $(objs) main.cpp
	$(CC) $(flags) -o $@ $^

run:
	./$(out) doc/hello.ck

debug:
	gdb ./$(out)

mem:
	valgrind -s --track-origins=yes --leak-check=full ./$(out)

clean:
	rm $(b)*.o
	rm $(out)

update:
	git stash
	git pull
	git stash apply

push:
	git add .
	git commit -m "$(m)"
	git push

# objs := $(b)AST.o $(b)ckript-vm.o $(b)error-handler.o $(b)evaluator.o $(b)interpreter.o $(b)lexer.o $(b)parser.o $(b)token.o $(b)utils.o