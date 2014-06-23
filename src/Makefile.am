
bin_PROGRAMS = single server client hybrid

COMMON_SOURCES = engine.cpp resource.cpp tokenizer.cpp profile.cpp object.cpp
CLIENT_SOURCES = graphics.cpp text.cpp model.cpp sound.cpp ui.cpp gldump.cpp
SERVER_SOURCES = game.cpp 
NETWORK_SOURCES = network.cpp smdp.cpp

single_SOURCES = single.cpp                $(COMMON_SOURCES) $(SERVER_SOURCES) $(CLIENT_SOURCES)  
client_SOURCES = clientmain.cpp client.cpp $(COMMON_SOURCES)                   $(CLIENT_SOURCES) $(NETWORK_SOURCES)
server_SOURCES = servermain.cpp server.cpp $(COMMON_SOURCES) $(SERVER_SOURCES) dummygraphics.cpp $(NETWORK_SOURCES)
hybrid_SOURCES = hybridmain.cpp client.cpp server.cpp $(COMMON_SOURCES) $(SERVER_SOURCES) $(CLIENT_SOURCES) $(NETWORK_SOURCES)
