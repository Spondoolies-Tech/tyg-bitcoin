TARGET :=		jam

SRCS :=			$(wildcard *.c)
OBJS :=			$(subst .c,.o,$(SRCS))

CC =			arm-none-linux-gnueabi-gcc

WARN_FLAGS :=		-Wall -Wextra
OPT_FLAGS :=		-O3
DBG_FLAGS :=		-s -fomit-frame-pointer

CFLAGS :=		$(WARN_FLAGS) $(OPT_FLAGS) $(DBG_FLAGS)

all:			$(TARGET)


$(TARGET):	$(OBJS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	$(RM) $(TARGET) $(OBJS)

