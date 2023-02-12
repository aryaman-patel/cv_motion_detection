CXX=@g++
CPPFLAGS=-std=c++17

src: src.cpp
	$(CXX) $(CPPFLAGS) -o src src.cpp $(shell pkg-config --cflags --libs opencv4)


run: src
	./src

clean:
	@rm -rf src *.o