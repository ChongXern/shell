CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -std=gnu11 -pthread

TARGET = promptshell
OBJS = promptshell.o threads.o commands.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

promptshell.o: promptshell.c promptshell.h
	$(CC) $(CFLAGS) -c promptshell.c

threads.o: threads.c threads.h
	$(CC) $(CFLAGS) -c threads.c

commands.o: commands.c commands.h
	$(CC) $(CFLAGS) -c commands.c

clean:
	rm -f $(TARGET) $(OBJS)
