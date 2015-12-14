TARGET=main
CXX=g++
CXXFLAGS=-Wall -g -std=c++11
#CXXFLAGS+=-O2
LDFLAGS=-lpthread -g

SOURCES=main.cpp Perf.cpp 

all: $(TARGET)

$(TARGET): $(SOURCES:%.cpp=%.o)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) 

%.o: %.cpp $(wildcard *.hpp)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

.PHONY: clean

clean: 
	\rm -f $(SOURCES:%.cpp=%.o)
	\rm -f $(TARGET)
