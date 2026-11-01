CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O3
TARGET = karnaugh_reducer
SOURCES = main.cpp
HEADERS = cube.hpp cover_search.hpp reducer.hpp

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run
