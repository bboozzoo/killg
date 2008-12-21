SRCS=main.cc

TARGET=killg
CXXFLAGS=$(shell sdl-config --cflags) -DUSE_OPENGL
LDFLAGS=$(shell sdl-config --libs) -lSDL_mixer -lSDL_image -lGL

$(TARGET): $(SRCS:.cc=.o)
	$(CXX) -o $(TARGET) $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) -c -o $@ $^ $(CXXFLAGS)

clean: 
	rm -f *.o
	rm -f $(TARGET)
	rm -f *~
