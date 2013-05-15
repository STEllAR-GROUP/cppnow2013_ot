CXX=g++

ifdef DEBUG
	CXXFLAGS+=-O0 -g
else
	CXXFLAGS+=-O3 -finline
endif

CXXFLAGS+=-std=c++11 
LIBS=-lboost_system -lboost_serialization
PROGRAMS=ot_client ot_server 
DIRECTORIES=build

all: directories $(PROGRAMS)

.PHONY: directories
directories: $(DIRECTORIES)/  

$(DIRECTORIES)/:
	mkdir -p $@ 

% : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) $< -o build/$@

clean:
	rm -rf $(DIRECTORIES) 

