CFLAG = -c -O0 -g -I /opt/ffmpeg/include 
LKFLAG = -lcurl -lpthread -L /opt/ffmpeg/lib 

OBJS = arduino_cmd.o serial-linux.o hardware.o slrov.o HttpStream.o HttpUrlConnection.o \
	  ServerConfig.o slclient.o slserver.o gopro4.o gopro_plan_queue.o main.o 

LKLIBA = -lavcodec -lavdevice -lavformat -lavutil -lswresample -lz 

slserver: $(OBJS)
	g++ -o $@ $(OBJS) $(LKFLAG) $(LKLIBA)
	
%.o:%.c
	gcc $(CFLAG) -o $@ $<
	
%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	rm *.o
	rm slserver
