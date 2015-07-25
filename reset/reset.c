#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
	int fd;
	int i=0;
	fd = open("/sys/class/gpio/export",O_WRONLY);
	write(fd,"30",2);
	close(fd);
	
	fd = open("/sys/class/gpio/gpio30/direction",O_WRONLY);
	write(fd,"out",3);
	close(fd);
	
	fd = open("/sys/class/gpio/gpio30/value",O_WRONLY);
	write(fd,"0",1);
	usleep(10);
	write(fd,"1",1);
	close(fd);
	
	fd = open("/sys/class/gpio/unexport",O_WRONLY);
	write(fd,"30",2);
	close(fd);
	return 0;
}