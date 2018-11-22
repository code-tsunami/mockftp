EXEC = ftserver
CC = gcc
ifeq ($(shell hostname),TheTsunami)
	CC = gcc-8
endif
CFLAGS = -Wall -std=gnu99

default: $(EXEC)

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) $(EXEC).c -o $(EXEC)

clean:
	rm -f $(EXEC)
