CFLAGS=-c -I . -g

all: config

config: http.o transceiver.o request.o protocol.o exceptions.o config.o db.o model.o logging.o util.o debug.o form.o
	g++ $^ -g -o $@ -lsqlite3

install: config
	rm -f /home/vhmauery/local/pyrobox/config.fcgi
	cp config /home/vhmauery/local/pyrobox/config.fcgi.tmp
	mv /home/vhmauery/local/pyrobox/config.fcgi.tmp /home/vhmauery/local/pyrobox/config.fcgi

http.o: http.cpp http.hpp util.hpp logging.hpp
	g++ http.cpp $(CFLAGS) -o $@

debug.o: debug.cpp debug.hpp logging.hpp
	g++ debug.cpp $(CFLAGS) -o $@

form.o: form.cpp form.hpp logging.hpp model.hpp db.hpp
	g++ form.cpp $(CFLAGS) -o $@

util.o: util.cpp util.hpp logging.hpp
	g++ util.cpp $(CFLAGS) -o $@

logging.o: logging.cpp logging.hpp
	g++ logging.cpp $(CFLAGS) -o $@

transceiver.o: transceiver.cpp transceiver.hpp util.hpp logging.hpp debug.hpp
	g++ transceiver.cpp $(CFLAGS) -o $@

request.o: request.cpp request.hpp util.hpp logging.hpp
	g++ request.cpp $(CFLAGS) -o $@

protocol.o: protocol.cpp protocol.hpp util.hpp logging.hpp
	g++ protocol.cpp $(CFLAGS) -o $@

exceptions.o: exceptions.cpp exceptions.hpp util.hpp logging.hpp
	g++ exceptions.cpp $(CFLAGS) -o $@

db.o: db.cpp db.hpp logging.hpp
	g++ db.cpp $(CFLAGS) -o $@

model.o: model.cpp model.hpp db.hpp logging.hpp
	g++ model.cpp $(CFLAGS) -o $@

config.o: config.cpp manager.hpp request.hpp util.hpp db.hpp logging.hpp debug.hpp form.hpp
	g++ config.cpp $(CFLAGS) -o $@

clean:
	rm -f *.o config

