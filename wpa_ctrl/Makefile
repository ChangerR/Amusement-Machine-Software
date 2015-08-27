CFLAG = -c -O0 -g 
LKFLAG =  

OBJS =	os_unix.o wpabuf.o wpa_debug.o common.o wpa_ctrl.o main.o 
LKLIBA = 

test: $(OBJS)
	gcc -o $@ $(OBJS) $(LKFLAG) $(LKLIBA)
	
%.o:%.c
	gcc $(CFLAG) -o $@ $<
	
%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	rm *.o
	rm test
