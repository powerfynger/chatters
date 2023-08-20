CC = gcc
CFLAGS = -Wall
LIBS = -lpthread -lncurses

SRCS = messages.c main.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = chatters

all: $(EXECUTABLE)
	@echo "Compilation complete."
	@$(MAKE) cleanobjs

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

messages.o: messages.c messages.h
	$(CC) $(CFLAGS) -c messages.c -o messages.o


clean:
	rm -f $(OBJS) $(EXECUTABLE)

cleanobjs:
	rm -f $(filter-out $(EXECUTABLE), $(OBJS))

rebuild: clean all