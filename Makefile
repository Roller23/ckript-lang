CC=g++
main=main.cpp
out=main.out
flags=-g -o0
files=lexer.cpp interpreter.cpp
m=Automated Makefile push message

all:
	$(CC) $(flags) -o $(out) $(main) $(files)

run:
	./$(out)

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