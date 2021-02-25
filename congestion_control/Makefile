OBJECTS=Sender.o Reliable.o ReliableImpl.o Util.o Queue.o Congestion.o
DEPS=Reliable.h ReliableImpl.h Util.h Queue.h Congestion.h

Sender: $(OBJECTS)
	gcc $(OBJECTS) -o Sender -lpthread -std=gnu99

Sender.o: Sender.c $(DEPS)
	gcc Sender.c -c -std=gnu99

Reliable.o: Reliable.c $(DEPS)
	gcc Reliable.c -c -std=gnu99

ReliableImpl.o: ReliableImpl.c $(DEPS)
	gcc ReliableImpl.c -c -std=gnu99

Congestion.o: Congestion.c $(DEPS)
	gcc Congestion.c -c -std=gnu99

Util.o: Util.c Util.h Queue.h
	gcc Util.c -c -std=gnu99

Queue.o: Queue.c Queue.h
	gcc Queue.c -c -std=gnu99

clean:
	rm *.o
