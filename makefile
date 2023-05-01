CC = g++
CFLAGS = -std=c++11 -Wall

SRCS = main.cpp BlockIO.cpp BlockRecovery.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = program

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) outfile.pptx outfile.zip outfile
