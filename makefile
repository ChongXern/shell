CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -std=gnu11 -pthread

TARGET = myshell
OBJS = myshell.o threads.o commands.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

myshell.o: myshell.c myshell.h
	$(CC) $(CFLAGS) -c myshell.c

threads.o: threads.c threads.h
	$(CC) $(CFLAGS) -c threads.c

commands.o: commands.c commands.h
	$(CC) $(CFLAGS) -c commands.c

clean:
	rm -f $(TARGET) $(OBJS)