CC := gcc
CFLAGS := -Wall -Wextra -std=c99

TARGET := salc
OBJDIR := dist
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
VALID_TESTS := tests/valid/basic.sal tests/valid/or_no_spaces.sal \
	tests/valid/negative_step.sal
INVALID_TESTS := tests/invalid/empty_char.sal \
	tests/invalid/multidim_vector.sal tests/invalid/unknown_character.sal \
	tests/invalid/ret_in_proc.sal

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)


$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

test: $(TARGET)
	@set -e; \
	for file in $(VALID_TESTS); do \
		./$(TARGET) "$$file" >/dev/null; \
	done; \
	for file in $(INVALID_TESTS); do \
		if ./$(TARGET) "$$file" >/dev/null 2>&1; then \
			printf 'Falha: %s deveria ser rejeitado.\n' "$$file" >&2; \
			exit 1; \
		fi; \
	done; \
	printf 'Todos os testes passaram.\n'

clean:
	rm -f $(OBJS) $(TARGET) compilador *.tk *.ts *.trc
	rmdir $(OBJDIR) 2>/dev/null || true
