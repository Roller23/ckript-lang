m :=Automated Makefile push message
CC := g++
bin := bin/
out := $(bin)ckript
flags := -O0 -g
src := src/
b := build/
obj_rule = $(CC) $(flags) -c $< -o $@

objs := $(shell find $(src) -name '*.cpp' | sed -e 's/.cpp/.o/g' | sed -e 's/src\//build\//g')

all: $(out)

build/AST.o: src/AST.cpp src/AST.hpp
	$(obj_rule)

build/ckript-vm.o: src/ckript-vm.cpp src/ckript-vm.hpp
	$(obj_rule)

build/error-handler.o: src/error-handler.cpp src/error-handler.hpp
	$(obj_rule)

build/evaluator.o: src/evaluator.cpp src/evaluator.hpp
	$(obj_rule)

build/interpreter.o: src/interpreter.cpp src/interpreter.hpp
	$(obj_rule)

build/lexer.o: src/lexer.cpp src/lexer.hpp
	$(obj_rule)

build/parser.o: src/parser.cpp src/parser.hpp
	$(obj_rule)

build/token.o: src/token.cpp src/token.hpp
	$(obj_rule)

build/utils.o: src/utils.cpp src/utils.hpp
	$(obj_rule)

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