# Memtester Makefile

# Written by Vladik Goytin

TARGET			:= memtester


SRCS			:= $(wildcard *.c)
OBJS			:= $(subst .c,.o,$(SRCS))

CC			:= arm-none-linux-gnueabi-gcc

WARN_FLAGS		:= -Wall -Wextra
OPT_FLAGS		:= -Os
CFLAGS			:= $(WARN_FLAGS) $(OPT_FLAGS)
LDFLAGS			:= -s -Wl,-O


all:			$(TARGET)

$(TARGET):		$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJS):		Makefile


.PHONY:	clean
clean:
	$(RM) $(TARGET) $(OBJS)
