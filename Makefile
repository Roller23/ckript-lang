m :=Automated Makefile push message
CC := g++
bin := bin/
out := $(bin)ckript
flags := -O3 -lm
src := src/
build := build/
objs := $(shell find $(src) -name '*.cpp' | sed -e 's/.cpp/.o/g' | sed -e 's/src\//build\//g')
intput := examples/hash_table.ck

all: $(out)

build/%.o: src/%.cpp src/%.hpp
	@mkdir -p $(build)
	$(CC) $(flags) -c $< -o $@

$(out): $(objs) main.cpp
	@mkdir -p $(bin)
	$(CC) $(flags) -o $@ $^

run:
	./$(out) $(intput)

shell:
	./$(out)

debug:
	gdb ./$(out)

mem:
	valgrind --track-origins=yes ./$(out) $(intput)

clean:
	rm $(build)*.o
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