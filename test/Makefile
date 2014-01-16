
CXX := g++
CXX_INCLUDE := ../include
CXXFLAGS := -std=c++0x -Wall -g -O0 -I$(CXX_INCLUDE)
LDFLAGS :=

TARGET := executortest
SRCS := executortest.cpp
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.depends)


.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.depends: %.cpp
	$(CXX) -M $(CXXFLAGS) $< > $@

-include $(DEPS)