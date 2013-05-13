CXX=g++

ifdef DEBUG
	CXXFLAGS+=-O0 -g
else
	CXXFLAGS+=-O3 -finline
endif

CXXFLAGS+=-std=c++11 
LIBS=-lboost_system -lboost_program_options -lboost_serialization -lboost_program_options
ADDITIONAL_SOURCES=portable_binary_iarchive.cpp portable_binary_oarchive.cpp server.cpp client.cpp
PROGRAMS=hello_world 
DIRECTORIES=build

all: directories $(PROGRAMS)

.PHONY: directories
directories: $(DIRECTORIES)/  

$(DIRECTORIES)/:
	mkdir -p $@ 

% : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) $(ADDITIONAL_SOURCES) $< -o build/$@

clean:
	rm -rf $(DIRECTORIES) 

