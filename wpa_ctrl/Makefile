CFLAG = -c -O0 -g
LKFLAG = -lpthread 
CPPFLAG = -c -O0 -g -I ..
OBJS =	os_unix.o wpabuf.o wpa_debug.o common.o wpa_ctrl.o ../wifi_manager.o test.o 
LKLIBA = 

test: $(OBJS)
	g++ -o $@ $(OBJS) $(LKFLAG) $(LKLIBA)
	
%.o:%.c
	gcc $(CFLAG) -o $@ $<
	
%.o:%.cpp
	g++ $(CPPFLAG) -o $@ $<

clean:
	rm $(OBJS) 
	rm test
