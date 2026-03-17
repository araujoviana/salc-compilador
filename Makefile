CC := gcc
CFLAGS := -Wall -Wextra -std=c99

TARGET := compilador
OBJDIR := dist
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)


$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(TARGET)
	rmdir $(OBJDIR) 2>/dev/null || true
