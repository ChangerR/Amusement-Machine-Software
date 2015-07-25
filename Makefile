CFLAG = -c -O0 -g 
LKFLAG = -lcurl -lpthread 

OBJS = arduino_cmd.o serial-linux.o hardware.o slrov.o HttpStream.o HttpUrlConnection.o \
	  ServerConfig.o slclient.o slserver.o gopro4.o main.o 

slserver: $(OBJS)
	g++ -o $@ $(OBJS) $(LKFLAG)
	
%.o:%.c
	gcc $(CFLAG) -o $@ $<
	
%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	rm *.o
	rm slserver
