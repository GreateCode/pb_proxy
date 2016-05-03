CC = g++
FLAG = -g 
INCLUDE = -I /root/ws_sample/ -I /root/ws_sample/cryptopp/ -I /root/ws_sample/ws_master
LIBDIR =
LIB = -lboost_system -lboost_chrono -lcryptopp -lpthread -lprotobuf
BIN = ../bin
TARGET = ws_sample
SRCS = b64.cpp CConfig.cpp pthread_sync.cpp network.cpp cws_proxy.cpp \
		web_socket.cpp debug_log.cpp up.pb.cpp protobuf_up.cpp memory_pool.cpp \
		main.cpp

$(TARGET):$(SRCS:.cpp=.o)
	$(CC) $(FLAG) $(LIBDIR) $(LIB) -o $@ $^
	-mv -f $@ $(BIN)

%.o:%.cpp
	$(CC) $(FLAG) $(INCLUDE) -c -o $@ $<

clean:
	-rm -f *.o *.d
