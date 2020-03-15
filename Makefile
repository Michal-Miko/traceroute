traceroute : main.o utils.o
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -o traceroute \
		main.o utils.o

main.o : main.cpp defs.h utils.h
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -c main.cpp

utils.o : utils.cpp utils.h
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -c utils.cpp

clean :
	rm traceroute main.o
