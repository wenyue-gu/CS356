OBJECTS=Sender.o Reliable.o ReliableImpl.o
DEPS=Reliable.h ReliableImpl.h Util.h SafeQueue.h

Sender: $(OBJECTS)
	g++ Sender.o Reliable.o ReliableImpl.o -o Sender -lpthread

Sender.o: Sender.cpp $(DEPS)
	g++ Sender.cpp -c

Reliable.o: Reliable.cpp $(DEPS)
	g++ Reliable.cpp -c

ReliableImpl.o: ReliableImpl.cpp $(DEPS)
	g++ ReliableImpl.cpp -c

