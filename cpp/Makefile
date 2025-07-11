CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = karnaugh_reducer
SOURCES = main.cpp
HEADERS = minterm.hpp node.hpp priority_queue.hpp reducer.hpp

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run 