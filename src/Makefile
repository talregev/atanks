.PHONY: all clean veryclean

MODULES = atanks.o beam.o button.o environment.o explosion.o fade.o files.o globaldata.o \
          missile.o perlin.o physobj.o player.o satellite.o sky.o tank.o team.o teleport.o virtobj.o \
          update.o network.o floattext.o land.o text.o client.o gameloop.o

CXX?=clang++
LIB=ar
WINDRES=
# FLAGS += -DDATA_DIR=\".\" -Wno-write-strings -DNETWORK -DTHREADS
FLAGS += -DDATA_DIR=\"${INSTALLDIR}\" -DNEW_GAMELOOP -Wno-write-strings -DNETWORK -DTHREADS -pthread -Wextra -pedantic -ggdb3
OUTPUT = ../atanks
WFLAGS = 
OFLAGS = 
LFLAGS +=
LDFLAGS +=  `allegro-config --libs` -lm -lpthread

CXXFLAGS += -Wall -Iinclude # -fprofile-arcs -ftest-coverage 

SRCS  = $(MODULES:.o=.cpp)
GLOBALS	= main.h imagedefs.h externs.h

all: 	$(OUTPUT)

clean:  
	rm -f *.o

veryclean: clean
	rm $(OUTPUT)

$(MODULES): Makefile

atanks.o: atanks.cpp globals.h main.h menucontent.h
	$(CXX) -c atanks.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

button.o: button.cpp button.h
	$(CXX) -c button.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

client.o: client.h client.cpp
	$(CXX) -c client.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

environment.o: environment.cpp environment.h
	$(CXX) -c environment.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

explosion.o: explosion.cpp explosion.h
	$(CXX) -c explosion.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

files.o: files.cpp files.h text.h text.cpp
	$(CXX) -c files.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

floattext.o: floattext.cpp floattext.h
	$(CXX) -c floattext.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

gameloop.o: gameloop.cpp atanks.cpp main.h
	$(CXX) -c gameloop.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

globaldata.o: globaldata.cpp globaldata.h
	$(CXX) -c globaldata.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

land.o: land.cpp land.h globaldata.h environment.h
	$(CXX) -c land.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

missile.o: missile.cpp missile.h
	$(CXX) -c missile.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

teleport.o: teleport.cpp teleport.h
	$(CXX) -c teleport.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

physobj.o: physobj.cpp physobj.h
	$(CXX) -c physobj.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

player.o: player.cpp player.h
	$(CXX) -c player.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

tank.o: tank.cpp tank.h
	$(CXX) -c tank.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

team.o: team.cpp team.h
	$(CXX) -c team.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

virtobj.o: virtobj.cpp virtobj.h
	$(CXX) -c virtobj.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

fade.o: fade.cpp
	$(CXX) -c fade.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

perlin.o: perlin.cpp
	$(CXX) -c perlin.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

sky.o: sky.cpp sky.h
	$(CXX) -c sky.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

satellite.o: satellite.cpp satellite.h
	$(CXX) -c satellite.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

text.o: text.cpp text.h
	$(CXX) -c text.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

update.o: update.cpp update.h
	$(CXX) -c update.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

network.o: network.cpp network.h
	$(CXX) -c network.cpp -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

%.o: %.cpp %.h
	$(CXX) -c $< -o $@ $(FLAGS) $(LFLAGS) $(OFLAGS) $(CXXFLAGS)

$(OUTPUT): $(OBJECTS) $(MODULES)
	$(CXX) $(MODULES) -o $(OUTPUT) $(FLAGS) $(LFLAGS) $(LDFLAGS) $(SFLAGS) $(CXXFLAGS)
	# strip $(OUTPUT)

# dependencies:
physobj.h: main.h virtobj.h globaldata.h
virtobj.h: main.h player.h
main.h: imagedefs.h externs.h
globaldata.h: main.h
player.h: main.h menu.h
tank.h: physobj.h
floattext.h: virtobj.h main.h environment.h
menu.h: globaldata.h
environment.h: main.h tank.h
files.h: globaldata.h environment.h 
globals.h: virtobj.h floattext.h physobj.h tank.h missile.h explosion.h player.h environment.h globaldata.h teleport.h decor.h beam.h
button.h: globaldata.h environment.h
team.h: globaldata.h
satellite.h: environment.h globaldata.h virtobj.h
beam.h: main.h virtobj.h physobj.h
missile.h: main.h physobj.h
teleport.h: main.h virtobj.h
decor.h: main.h physobj.h environment.h globaldata.h
explosion.h: main.h physobj.h
virtobj.cpp: virtobj.h environment.h
physobj.cpp: physobj.h environment.h
atanks.cpp: globals.h menu.h button.h team.h files.h satellite.h menucontent.h
beam.cpp: environment.h globaldata.h physobj.h player.h decor.h tank.h beam.h
button.cpp: button.h
environment.cpp: environment.h globaldata.h virtobj.h missile.h tank.h files.h
explosion.cpp: environment.h globaldata.h explosion.h missile.h decor.h tank.h player.h
fade.cpp: globaldata.h main.h
files.cpp: player.h files.h main.h
globaldata.cpp: player.h globaldata.h files.h
missile.cpp: environment.h globaldata.h explosion.h missile.h decor.h tank.h
perlin.cpp: main.h
player.cpp: environment.h globaldata.h player.h tank.h menu.h files.h floattext.h
satellite.cpp: environment.h satellite.h beam.h
sky.cpp: globaldata.h main.h sky.h
tank.cpp: environment.h globaldata.h floattext.h explosion.h teleport.h missile.h player.h beam.h tank.h
team.cpp: tank.h team.h player.h
teleport.cpp: environment.h globaldata.h teleport.h
