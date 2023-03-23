CC := g++
CFLAGS := -Wall -g
INCLUDE_PATH := ./
GLFW := $(shell pkg-config --libs glfw3)
LIBS :=  -lGLEW -lGLU -lm -lGL -lm -lpthread -lrt -ldl $(GLFW) -ljpeg

TARGET := blur

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)
	
%.o: %.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< 
	
clean:
	rm -rf $(TARGET) *.o