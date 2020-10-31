CC=g++
out=main.out
flags=-g -O0
files=*.cpp
m=Automated Makefile push message
input_file=call.ck

all:
	$(CC) $(flags) -o $(out) $(files)

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