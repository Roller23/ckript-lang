CC=g++
main=main.cpp
out=main.out
flags=-g -O0
files=token.cpp lexer.cpp parser.cpp ckript-vm.cpp interpreter.cpp AST.cpp error-handler.cpp
m=Automated Makefile push message
input_file=test.ck

all:
	$(CC) $(flags) -o $(out) $(main) $(files)

run:
	./$(out) $(input_file)

debug:
	gdb ./$(out)

clean:
	rm *.out

update:
	git stash
	git pull
	git stash apply

push:
	git add .
	git commit -m "$(m)"
	git push