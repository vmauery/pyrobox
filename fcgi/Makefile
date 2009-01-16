CFLAGS=-c -I . -g

all: config

config: http.o transceiver.o request.o protocol.o exceptions.o config.o util.o db.o model.o
	g++ $^ -g -o $@ -lsqlite3

install: config
	rm -f /home/vhmauery/local/pyrobox/config.fcgi
	cp config /home/vhmauery/local/pyrobox/config.fcgi.tmp
	mv /home/vhmauery/local/pyrobox/config.fcgi.tmp /home/vhmauery/local/pyrobox/config.fcgi

http.o: http.cpp http.hpp utf8_codecvt.hpp utf8_codecvt_facet.hpp util.hpp
	g++ http.cpp $(CFLAGS) -o $@

transceiver.o: transceiver.cpp transceiver.hpp util.hpp
	g++ transceiver.cpp $(CFLAGS) -o $@

request.o: request.cpp utf8_codecvt_facet.cpp request.hpp utf8_codecvt.hpp utf8_codecvt_facet.hpp util.hpp
	g++ request.cpp $(CFLAGS) -o $@

protocol.o: protocol.cpp protocol.hpp util.hpp
	g++ protocol.cpp $(CFLAGS) -o $@

exceptions.o: exceptions.cpp exceptions.hpp util.hpp
	g++ exceptions.cpp $(CFLAGS) -o $@

util.o: util.cpp util.hpp
	g++ util.cpp $(CFLAGS) -o $@

db.o: db.cpp db.hpp
	g++ db.cpp $(CFLAGS) -o $@

model.o: model.cpp model.hpp db.hpp
	g++ model.cpp $(CFLAGS) -o $@

config.o: config.cpp manager.hpp request.hpp util.hpp db.hpp
	g++ config.cpp $(CFLAGS) -o $@

clean:
	rm -f *.o config

