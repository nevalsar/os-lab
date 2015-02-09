run: parent.out child.out
	./parent.out
parent.out: parent.c child.out
	gcc -g -Wall parent.c -o parent.out

child.out: child.c
	gcc -g -Wall child.c -o child.out

clean:
	rm child.out parent.out;