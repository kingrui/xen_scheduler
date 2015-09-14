.PHONY:aa
#aa:Main.cpp SysInfo.cpp SysInfo.h Scheduler.cpp Scheduler.h kmeans.cpp kmeans.h runnable.cpp runnable.h 
#       g++ -o aa -std=c++11 Main.cpp SysInfo.cpp SysInfo.h Scheduler.cpp Scheduler.h kmeans.cpp kmeans.h runnable.cpp runnable.h  -lvirt -lxenctrl -pthread -D_GLIBCXX_USE_NANOSLEEP

aa:Main.o SysInfo.o Scheduler.o kmeans.o runnable.o Vnuma.o
		g++ -o aa -std=c++11 kmeans.o  Main.o  Scheduler.o  SysInfo.o runnable.o Vnuma.o -lvirt -lxenctrl -pthread -D_GLIBCXX_USE_NANOSLEEP
		make clean
Main.o: Main.cpp SysInfo.h Scheduler.h
		g++ -c -std=c++11 Main.cpp
SysInfo.o:SysInfo.cpp SysInfo.h runnable.h
		g++ -c -std=c++11 SysInfo.cpp -D_GLIBCXX_USE_NANOSLEEP
Scheduler.o:Scheduler.cpp Scheduler.h
		g++ -c -std=c++11 Scheduler.cpp -D_GLIBCXX_USE_NANOSLEEP
Vnuma.o:Vnuma.cpp Vnuma.h
		g++ -c -std=c++11 Vnuma.cpp -D_GLIBCXX_USE_NANOSLEEP
kmeans.o:kmeans.cpp kmeans.h
		g++ -c -std=c++11 kmeans.cpp
runnable.o:runnable.cpp runnable.h
		g++ -c -std=c++11 runnable.cpp
clean:
		rm *.o