#ifndef __SLSERVER_POLIT_H
#define __SLSERVER_POLIT_H
#include "slconfig.h"

class slrov;

class Pilot {
public:
	Pilot(slrov* pointer);
	virtual ~Pilot(){};
	
	void setHeadingHold(bool,int);
	
	void setDepthHold(bool,int);
	
	void pilotdo(int*,int*,int*,int*);
	
private:
	slrov* rov;
	bool _depthHoldEnabled;
	bool _headingHoldEnabled;
	int _depth_deadband; // +/- cm
	int _heading_deadband;  // +/i degrees
	int _headingHoldTarget,_depthHoldTarget;
	float _heading_loop_gain;
	float _depth_hold_loop_gain;
	int _target_depth;
	int _hdg_Error,_depth_Error;
	int _raw_Left, _raw_Right,_raw_yaw;
	long _hdg_Error_Integral;
	int _tgt_Hdg;
	int _raw_lift;
};
#endif