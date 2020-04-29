CC = -std=c99

main: main.c queue.c
	gcc $(CC) main.c queue.c -o main

clean:
	rm main