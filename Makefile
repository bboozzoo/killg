SRCS=main.cc

TARGET=killg
CXXFLAGS=$(shell sdl-config --cflags) -O2 -fomit-frame-pointer #-DLOG_ENABLE
LDFLAGS=$(shell sdl-config --libs) -lSDL_mixer -lGL -lSOIL

$(TARGET): $(SRCS:.cc=.o)
	$(CXX) -o $(TARGET) $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) -c -o $@ $^ $(CXXFLAGS)

clean: 
	rm -f *.o
	rm -f $(TARGET)
	rm -f *~
