traceroute : main.o traceroute.o
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -o traceroute \
		main.o traceroute.o

main.o : main.cpp traceroute.h
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -c main.cpp

traceroute.o : traceroute.cpp traceroute.h
	g++ -std=c++17 -Wall -Wextra -Weffc++ -Wshadow -O2 -static -c traceroute.cpp

clean:
	rm main.o traceroute.o

distclean:
	rm traceroute main.o traceroute.o
