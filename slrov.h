#ifndef __SL_SLROV_H
#define __SL_SLROV_H
#include "hardware.h"
#include <pthread.h>
#define MAX_CLIENT_CMD_LEN 256
#define MAX_CLIENT_ARGS 16

#define PORT_INDEX 0
#define STARBORD_INDEX 1
#define VERTICAL_LEFT_INDEX 2
#define VERTICAL_RIGHT_INDEX 3

#define POWER0_INDEX 0
#define POWER1_INDEX 4
#define POWER2_INDEX 8
#define POWER3_INDEX 12
#define POWER4_INDEX 16
#define POWER5_INDEX 20
#define POWER_INDEX(a) ((a)*4)

class SlServer;
class slrov {
public:
	slrov(SlServer* pointer);
	virtual ~slrov();

	bool start(const char* s);
	int runcommand(int ,const char* );
	bool parseCommand(const char*);
	void stop();

//#ifdef SLSERVER_WIN32
	static void* __cdecl pid(void*);
//#else
//	static void* pid(void*);
//#endif

	SlServer* getServer() {
		return server;
	}
	
	void setMpuEular(float _x,float _y,float _z);
	
	void setMs5803_data(float temp,float press);
private:
	hardware* rov;
	SlServer* server;
	pthread_t _thread;
	bool running;
	char cmd[MAX_CLIENT_CMD_LEN];
	char argument[MAX_CLIENT_ARGS][MAX_CLIENT_CMD_LEN];
	int args;
	int thr,yaw,lift;
	int port,starboard,vertical_left,vertical_right;
	float mpu_roll,mpu_pitch,mpu_yaw;
	float ms5803_temp,ms5803_press;
	float depth,mpu_campass;

	int _midpoint[4];
	int _power_delta[24];
	int _yaws_stable[6];
	
};

#endif
