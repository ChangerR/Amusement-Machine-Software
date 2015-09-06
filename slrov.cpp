#ifdef __linux__
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#include <unistd.h>
#endif
#include "slconfig.h"
#include "slrov.h"
#include "slserver.h"
#include "arduino_cmd.h"
#include "ServerConfig.h"
#include <errno.h>


#define PARSE_COMMAND 0
#define PARSE_ARGS    1
#define PARSE_STRING  2
#define PARSE_ERROR   3
#define PARSE_END     4

#define MIDPOINT 1500
int delta_power[] = {50,100,150,200,250};
int delta_yaw[] = {10,20,30,40,60};

#define _ABS(a) (((a) >= 0)?(a):-(a))
#define _NEG(a) (((a) >= 0)?1:-1)
#define _DERICTION(a) (((a) >= 0)?0:1)

const float Default_AtmosPressure = 1015.f;
const float Default_WaterDensity = 1.019716f;
	
slrov::slrov(SlServer* pointer) {
	rov = NULL;
	server = pointer;
	thr = yaw = lift = 0;
	port = starboard = vertical_left = vertical_right = MIDPOINT;
	
	pilot = new Pilot(this);
}

slrov::~slrov() {
	
}

int arduino_log(int args, char(*argv)[MAX_CMD_ARGUMENT_LEN], void* user) {
	LOGOUT("***INFO*** SERIAL LOG:%s\n", argv[0]);
	((slrov*)user)->getServer()->broadcast(3, argv[0]);
	return 1;
}

int recv_mpu9150(int args, char(*argv)[MAX_CMD_ARGUMENT_LEN], void* user) {
	float _roll,_pitch,_yaw;
	if(args != 3)
		return -1;
	_roll = parse_float(argv[0]);
	_pitch = parse_float(argv[1]);
	_yaw = parse_float(argv[2]);
	//printf("***INFO*** EULAR %f,%f,%f\n",_roll,_pitch,_yaw);
	((slrov*)user)->setMpuEular(_roll,_pitch,_yaw);
	return 0;
}

int recv_ms5803(int args, char(*argv)[MAX_CMD_ARGUMENT_LEN], void* user) {
	float _temp,_press;
	if(args != 2)
		return -1;
	_temp = parse_float(argv[0]);
	_press = parse_float(argv[1]);

	//printf("***INFO*** MS5803 %f,%f\n",_temp,_press);
	((slrov*)user)->setMs5803_data(_temp,_press);
	return 0;
}

void slrov::setMpuEular(float _x,float _y,float _z) {
	char tmp[128];
	mpu_roll = _x;
	mpu_pitch = _y;
	mpu_yaw = _z;
	mpu_campass = _z;

	if (mpu_campass < 0)mpu_campass += 360.f;

	sprintf_s(tmp,"hdgd=%.2f,roll=%.2f,pitch=%.2f,yaw=%.2f",mpu_campass,mpu_roll,mpu_pitch,mpu_yaw);
	server->broadcast(7,tmp);
	//printf("***INFO*** %s\n", tmp);
}

void slrov::setMs5803_data(float temp,float press) {
	char tmp[128];
	ms5803_temp = temp;
	ms5803_press = press;
	depth = (ms5803_press - AtmosPressure) * WaterDensity / 100;
	
	sprintf_s(tmp,"temp=%.2f,depth=%.2f",temp,depth);
	server->broadcast(7,tmp);
	//printf("***INFO*** %s\n", tmp);
}
	
bool slrov::start(const char* s) {
	running = true;
	rov = new hardware();
	
	if (server->_config)
	{
		_power_delta[0] = _power_delta[1] = _power_delta[2] =  _power_delta[3] = 0;
		
		if (!server->_config->getInt("PORT_MIDPOINT", &_midpoint[PORT_INDEX]))
			_midpoint[PORT_INDEX] = MIDPOINT;

		if (!server->_config->getInt("STARBORD_MIDPOINT", &_midpoint[STARBORD_INDEX]))
			_midpoint[STARBORD_INDEX] = MIDPOINT;

		if (!server->_config->getInt("VERTICAL_LEFT", &_midpoint[VERTICAL_LEFT_INDEX]))
			_midpoint[VERTICAL_LEFT_INDEX] = MIDPOINT;

		if (!server->_config->getInt("VERTICAL_RIGHT", &_midpoint[VERTICAL_RIGHT_INDEX]))
			_midpoint[VERTICAL_RIGHT_INDEX] = MIDPOINT;
		
		if (!server->_config->getInt("POWER1_PORT", &_power_delta[POWER1_INDEX + PORT_INDEX]))
			_power_delta[POWER1_INDEX + PORT_INDEX] = delta_power[0];
		
		if (!server->_config->getInt("POWER1_STARBORD", &_power_delta[POWER1_INDEX + STARBORD_INDEX]))
			_power_delta[POWER1_INDEX + STARBORD_INDEX] = delta_power[0];
		
		if (!server->_config->getInt("POWER1_VERTICAL_LEFT", &_power_delta[POWER1_INDEX + VERTICAL_LEFT_INDEX]))
			_power_delta[POWER1_INDEX + VERTICAL_LEFT_INDEX] = delta_power[0];
		
		if (!server->_config->getInt("POWER1_VERTICAL_RIGHT", &_power_delta[POWER1_INDEX + VERTICAL_RIGHT_INDEX]))
			_power_delta[POWER1_INDEX + VERTICAL_RIGHT_INDEX] = delta_power[0];
		
		if (!server->_config->getInt("POWER2_PORT", &_power_delta[POWER2_INDEX + PORT_INDEX]))
			_power_delta[POWER2_INDEX + PORT_INDEX] = delta_power[1];
		
		if (!server->_config->getInt("POWER2_STARBORD", &_power_delta[POWER2_INDEX + STARBORD_INDEX]))
			_power_delta[POWER2_INDEX + STARBORD_INDEX] = delta_power[1];
		
		if (!server->_config->getInt("POWER2_VERTICAL_LEFT", &_power_delta[POWER2_INDEX + VERTICAL_LEFT_INDEX]))
			_power_delta[POWER2_INDEX + VERTICAL_LEFT_INDEX] = delta_power[1];
		
		if (!server->_config->getInt("POWER2_VERTICAL_RIGHT", &_power_delta[POWER2_INDEX + VERTICAL_RIGHT_INDEX]))
			_power_delta[POWER2_INDEX + VERTICAL_RIGHT_INDEX] = delta_power[1];
		
		if (!server->_config->getInt("POWER3_PORT", &_power_delta[POWER3_INDEX + PORT_INDEX]))
			_power_delta[POWER3_INDEX + PORT_INDEX] = delta_power[2];
		
		if (!server->_config->getInt("POWER3_STARBORD", &_power_delta[POWER3_INDEX + STARBORD_INDEX]))
			_power_delta[POWER3_INDEX + STARBORD_INDEX] = delta_power[2];
		
		if (!server->_config->getInt("POWER3_VERTICAL_LEFT", &_power_delta[POWER3_INDEX + VERTICAL_LEFT_INDEX]))
			_power_delta[POWER3_INDEX + VERTICAL_LEFT_INDEX] = delta_power[2];
		
		if (!server->_config->getInt("POWER3_VERTICAL_RIGHT", &_power_delta[POWER3_INDEX + VERTICAL_RIGHT_INDEX]))
			_power_delta[POWER3_INDEX + VERTICAL_RIGHT_INDEX] = delta_power[2];
		
		if (!server->_config->getInt("POWER4_PORT", &_power_delta[POWER4_INDEX + PORT_INDEX]))
			_power_delta[POWER4_INDEX + PORT_INDEX] = delta_power[3];
		
		if (!server->_config->getInt("POWER4_STARBORD", &_power_delta[POWER4_INDEX + STARBORD_INDEX]))
			_power_delta[POWER4_INDEX + STARBORD_INDEX] = delta_power[3];
		
		if (!server->_config->getInt("POWER4_VERTICAL_LEFT", &_power_delta[POWER4_INDEX + VERTICAL_LEFT_INDEX]))
			_power_delta[POWER4_INDEX + VERTICAL_LEFT_INDEX] = delta_power[3];
		
		if (!server->_config->getInt("POWER4_VERTICAL_RIGHT", &_power_delta[POWER4_INDEX + VERTICAL_RIGHT_INDEX]))
			_power_delta[POWER4_INDEX + VERTICAL_RIGHT_INDEX] = delta_power[3];
		
		if (!server->_config->getInt("POWER5_PORT", &_power_delta[POWER5_INDEX + PORT_INDEX]))
			_power_delta[POWER5_INDEX + PORT_INDEX] = delta_power[4];
		
		if (!server->_config->getInt("POWER5_STARBORD", &_power_delta[POWER5_INDEX + STARBORD_INDEX]))
			_power_delta[POWER5_INDEX + STARBORD_INDEX] = delta_power[4];
		
		if (!server->_config->getInt("POWER5_VERTICAL_LEFT", &_power_delta[POWER5_INDEX + VERTICAL_LEFT_INDEX]))
			_power_delta[POWER5_INDEX + VERTICAL_LEFT_INDEX] = delta_power[4];
		
		if (!server->_config->getInt("POWER5_VERTICAL_RIGHT", &_power_delta[POWER5_INDEX + VERTICAL_RIGHT_INDEX]))
			_power_delta[POWER5_INDEX + VERTICAL_RIGHT_INDEX] = delta_power[4];
		
		if (!server->_config->getFloat("PORT_SCALE_FACT_FORWARD", &_scale_fact[0]))
			_scale_fact[0] = 1.f;
		
		if (!server->_config->getFloat("PORT_SCALE_FACT_BACK", &_scale_fact[1]))
			_scale_fact[1] = -1.f;
		
		if (!server->_config->getFloat("STARBORD_SCALE_FACT_FORWARD", &_scale_fact[2]))
			_scale_fact[2] = 1.f;
		
		if (!server->_config->getFloat("STARBORD_SCALE_FACT_BACK", &_scale_fact[3]))
			_scale_fact[3] = -1.f;
		
		if (!server->_config->getFloat("VERTICAL_LEFT_SCALE_FACT_FORWARD", &_scale_fact[4]))
			_scale_fact[4] = 1.f;
		
		if (!server->_config->getFloat("VERTICAL_LEFT_SCALE_FACT_BACK", &_scale_fact[5]))
			_scale_fact[5] = -1.f;
		
		if (!server->_config->getFloat("VERTICAL_RIGHT_SCALE_FACT_FORWARD", &_scale_fact[6]))
			_scale_fact[6] = 1.f;
		
		if (!server->_config->getFloat("VERTICAL_RIGHT_SCALE_FACT_BACK", &_scale_fact[7]))
			_scale_fact[7] = -1.f;
		
		_yaws_stable[0] = 0;

		if (!server->_config->getInt("POWER1_YAW", &_yaws_stable[1]))
			_yaws_stable[1] = delta_yaw[0];
		
		if (!server->_config->getInt("POWER2_YAW", &_yaws_stable[2]))
			_yaws_stable[2] = delta_yaw[1];
		
		if (!server->_config->getInt("POWER3_YAW", &_yaws_stable[3]))
			_yaws_stable[3] = delta_yaw[2];
		
		if (!server->_config->getInt("POWER4_YAW", &_yaws_stable[4]))
			_yaws_stable[4] = delta_yaw[3];
		
		if (!server->_config->getInt("POWER5_YAW", &_yaws_stable[5]))
			_yaws_stable[5] = delta_yaw[4];
		
		if (!server->_config->getFloat("ATMOSPRESSURE", &AtmosPressure))
			AtmosPressure = Default_AtmosPressure;
		
		if (!server->_config->getFloat("WATERDENSITY", &WaterDensity))
			WaterDensity = Default_WaterDensity;
	} else {
		printf("***ERROR*** Init Config file error\n");
		return false;
	}
	port = _midpoint[PORT_INDEX];
	starboard = _midpoint[STARBORD_INDEX];
	vertical_left = _midpoint[VERTICAL_LEFT_INDEX];
	vertical_right = _midpoint[VERTICAL_RIGHT_INDEX];

	if(false == rov->init(s)) {
		printf("***ERROR*** Init hardware failed\n");
		return false;
	}
	
	if (0 != pthread_create(&_thread, NULL, slrov::pid, (void*)this)) {
		printf("**ERROR*** When create pthread watcher thread,%d\n", errno);
		return false;
	}
	rov->on("log", arduino_log, this);
	rov->on("MPU9150", recv_mpu9150, this);
	rov->on("ms5803", recv_ms5803, this);
	return true;
}

void slrov::stop() {
	delete rov;
	rov = NULL;
	running = false;
	pthread_join(_thread,NULL);
}

void slrov::motor_go(int _p,int _s,int _lv,int _rv) {
	float delta;
	int p,s,lv,rv;
	char buf[256];
	delta = _p - _midpoint[PORT_INDEX];
	p = delta >= 0.f ? (int(delta * _scale_fact[0]) + _midpoint[PORT_INDEX]):(int(delta * _scale_fact[1] * -1) + _midpoint[PORT_INDEX]);
	delta = _s - _midpoint[STARBORD_INDEX];
	s = delta >= 0.f ? (int(delta * _scale_fact[2]) + _midpoint[STARBORD_INDEX]):(int(delta * _scale_fact[3] * -1) + _midpoint[STARBORD_INDEX]);
	delta = _lv - _midpoint[VERTICAL_LEFT_INDEX];
	lv = delta >= 0.f ? (int(delta * _scale_fact[4]) + _midpoint[VERTICAL_LEFT_INDEX]):(int(delta * _scale_fact[5] * -1) + _midpoint[VERTICAL_LEFT_INDEX]);
	delta = _rv - _midpoint[VERTICAL_RIGHT_INDEX];
	rv = delta >= 0.f ? (int(delta * _scale_fact[6]) + _midpoint[VERTICAL_RIGHT_INDEX]):(int(delta * _scale_fact[7] * -1) + _midpoint[VERTICAL_RIGHT_INDEX]);
	
	sprintf_s(buf,"go(%d,%d,%d,%d)\r\n",p,s,lv,rv);
	rov->write(buf);
}

void* slrov::pid(void* user) {
	slrov* pointer = (slrov*)user;
	int _port = 0;
	int _starboard = 0;
	int _vertical_left = 0;
	int _vertical_right = 0;

	while(pointer->running) {
		if(_ABS(pointer->thr) <= 5&&_ABS(pointer->yaw) <= 5
			&&_ABS(pointer->lift) <= 5) {	
			
			_port = pointer->_power_delta[POWER_INDEX(_ABS(pointer->thr)) + PORT_INDEX] * _NEG(pointer->thr) + pointer->_midpoint[PORT_INDEX];
			_starboard = pointer->_power_delta[POWER_INDEX(_ABS(pointer->thr)) + STARBORD_INDEX] * _NEG(pointer->thr) + pointer->_midpoint[STARBORD_INDEX];
			
			if(pointer->thr != 0) {
				_port += pointer->_yaws_stable[_ABS(pointer->yaw)] * _NEG(pointer->yaw);
				_starboard -= pointer->_yaws_stable[_ABS(pointer->yaw)] * _NEG(pointer->yaw);
			}else{
				_port += pointer->_power_delta[POWER_INDEX(_ABS(pointer->yaw)) + PORT_INDEX] * _NEG(pointer->yaw);
				_starboard -= pointer->_power_delta[POWER_INDEX(_ABS(pointer->yaw)) + STARBORD_INDEX] * _NEG(pointer->yaw);
			}
			
			_vertical_left = pointer->_power_delta[POWER_INDEX(_ABS(pointer->lift)) + VERTICAL_LEFT_INDEX] * _NEG(pointer->lift) + pointer->_midpoint[VERTICAL_LEFT_INDEX];
			_vertical_right = pointer->_power_delta[POWER_INDEX(_ABS(pointer->lift)) + VERTICAL_RIGHT_INDEX] * _NEG(pointer->lift) + pointer->_midpoint[VERTICAL_RIGHT_INDEX];
						
		}
////////////////////////////////////////////
//this area you can do your pid thing
		pointer->pilot->pilotdo(&_port,&_starboard,&_vertical_left,&_vertical_right);
//////////////////////////////////////////
		if(_port != pointer->port || _starboard != pointer->starboard || pointer->vertical_left != _vertical_left
					||pointer->vertical_right != _vertical_right) {
				pointer->port = _port;
				pointer->starboard = _starboard;
				pointer->vertical_left = _vertical_left;
				pointer->vertical_right = _vertical_right;
				pointer->motor_go(pointer->port,pointer->starboard,pointer->vertical_left,pointer->vertical_right);
		}
#ifdef SLSERVER_WIN32 
		Sleep(1);
#elif defined(SLSERVER_LINUX)
		//sched_yield();
		usleep(1);
#endif
	}
	
	return NULL;
}

int slrov::runcommand(int uid,const char* p) {

	LOGOUT("***INFO*** UID:%d RUN:%s\n", uid, p);
	
	if(parseCommand(p) == false) {
		LOGOUT("**ERROR*** UID:%d SEND ERROR CMD:%s\n", uid, p);
		return -1;
	}
	
	switch(cmd[0]) {
		case 'g':
			if(cmd[1] == 'o'&&cmd[2] == 0) {
				thr = parse_int_dec(argument[0]);
				yaw = parse_int_dec(argument[1]);
				lift = parse_int_dec(argument[2]);
			}
			break;
		default:
			break;
	}
	
	return -1;
}


bool slrov::parseCommand(const char* p) {
	
	int parseState = PARSE_COMMAND;
	int offset = 0;
	args = 0;
	while(*p) {
		if(offset >= MAX_CLIENT_CMD_LEN)
			parseState = PARSE_ERROR;
		
		switch(parseState) {
		case PARSE_COMMAND:
		{	
			if(*p == '(') {
				cmd[offset++] = 0;
				parseState = PARSE_ARGS;
				offset = 0;
			}else 
				cmd[offset++] = *p;			
		}
			break;	
		case PARSE_ARGS:
		{
			if(*p == ',') {
				argument[args][offset] = 0;
				args++;
				offset = 0;
				if(args >= MAX_CLIENT_ARGS)
					parseState = PARSE_ERROR;
			} else if(*p == ')') {
				argument[args][offset] = 0;
				args++;
				parseState = PARSE_END;
			} else if(*p == '\"') {
				parseState = PARSE_STRING;
			} else if(*p != ' ') {
				argument[args][offset++] = *p;
			}
		}
			break;
		case PARSE_STRING:
		{
			if(*p == '\"')
				parseState = PARSE_ARGS;
			else
				argument[args][offset++] = *p;
		}
			break;
		default:
			break;
		}	
		if(parseState == PARSE_END || parseState == PARSE_ERROR)
			break;
		p++;
	}
	
	if(parseState == PARSE_END)
		return true;
	else
		return false;
}
