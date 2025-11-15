CC = gcc
CFLAGS = -Iinclude -std=c17 -Wall -Wextra
LDFLAGS = -Linclude -lkuta -lcglm -lm -Wl,-rpath,'$$ORIGIN/include'

TARGET = lightDiffuse
SOURCE = main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
