CFLAG = -c -O0 -g -I /opt/ffmpeg/include 
LKFLAG = -lcurl -lpthread -L /opt/ffmpeg/lib 

OBJS = arduino_cmd.o serial-linux.o hardware.o slrov.o HttpStream.o HttpUrlConnection.o \
	  ServerConfig.o slclient.o slserver.o gopro4.o gopro_plan_queue.o main.o wifi_manager.o pilot.o  

WPA_OBJS =	wpa_ctrl/os_unix.o wpa_ctrl/wpabuf.o wpa_ctrl/wpa_debug.o wpa_ctrl/common.o wpa_ctrl/wpa_ctrl.o 

LKLIBA = -lavcodec -lavdevice -lavformat -lavutil -lswresample -lz 

slserver: $(OBJS) $(WPA_OBJS)
	g++ -o $@ $(OBJS) $(WPA_OBJS) $(LKFLAG) $(LKLIBA)
	
%.o:%.c
	gcc $(CFLAG) -o $@ $<
	
%.o:%.cpp
	g++ $(CFLAG) -I wpa_ctrl -o $@ $<

clean:
	rm *.o
	rm slserver
