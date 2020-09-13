app: main.o server.o cJSON.o db.o
	$(CC) main.o server.o cJSON.o db.o -o app -g

main.o: src/main.c src/epoll_util.h
	$(CC) -c src/main.c

server.o: src/server.c src/server.h
	$(CC) -c src/server.c

cJSON.o: src/cJSON/cJSON.c src/cJSON/cJSON.h
	$(CC) -c src/cJSON/cJSON.c

db.o: src/db/item.c src/db/item.h src/db/db.c src/db/db.h
	$(CC) -c src/db/item.c src/db/db.c

clean:
	-rm -f *.o