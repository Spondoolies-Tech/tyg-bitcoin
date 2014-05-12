# 
#
# Written by Vladik Goytin

TARGET :=		ipaddr_state

SRCS :=			$(wildcard *.c)
OBJS :=			$(subst .c,.o,$(SRCS))

OPT_FLAGS :=		-Os 
WARN_FLAGS :=		-Wall -Wextra

CFLAGS +=		$(OPT_FLAGS) $(WARN_FLAGS)
LDFLAGS :=		-s

all:			$(TARGET)

$(TARGET):		$(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS)
