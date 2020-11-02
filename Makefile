CC=g++
out=main.out
production_flags=-g -O0
release_flags=-O3
files=*.cpp
m=Automated Makefile push message
input_file=call.ck

production:
	$(CC) $(production_flags) -o $(out) $(files)

release:
	$(CC) $(release_flags) -o $(out) $(files)

run:
	./$(out) $(input_file)

rerun:
	make production
	make run

debug:
	gdb ./$(out)

mem:
	valgrind -s ./$(out)

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