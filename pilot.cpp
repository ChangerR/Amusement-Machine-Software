#include "pilot.h"
#include "slrov.h"
#include "util.h"

Pilot::Pilot(slrov* pointer):rov(pointer) {
	_depth_deadband = 10; // +/- cm
	_heading_deadband = 10;  // +/i degrees
	_headingHoldTarget = 0;
	_heading_loop_gain = 1.0f;
	_depth_hold_loop_gain = 0.6f;//quanshi 
	_hdg_Error_Integral = 0;
	_tgt_Hdg = 0;
	_raw_lift =0;
	
	setHeadingHold(false,0);
	setDepthHold(false,0);
}

void Pilot::setHeadingHold(bool s,int hgdg) {
	if(!s){
		_headingHoldEnabled = false;
		_raw_Left = 0;
		_raw_Right = 0;
		_hdg_Error_Integral = 0;  // Reset error integrator
		_tgt_Hdg = -500;  // -500 = system not in hdg hold
	}else{
		_headingHoldEnabled = true;
		_headingHoldTarget = hgdg;
		_tgt_Hdg = _headingHoldTarget;
	}
}
	
void Pilot::setDepthHold(bool s,int d) {
	if(!s) {
		_depthHoldEnabled = false;
		_raw_lift = 0;
		_target_depth = 0;
	}else{
		_depthHoldEnabled = true;
		_depthHoldTarget = d;
		_target_depth = _depthHoldTarget;
	}
}

void Pilot::pilotdo(int* _p,int* _s,int* _pv,int* _sv) {
	
	if (_depthHoldEnabled)
	{
		_depth_Error = _target_depth - rov->depth*100;  //positive error = positive lift = go deaper.

		_raw_lift = (float)_depth_Error * _depth_hold_loop_gain;
		_raw_lift = sl_constrain(_raw_lift, -400, 400);

		if (sl_abs(_depth_Error) > _depth_deadband){
			*_pv = rov->_midpoint[VERTICAL_LEFT_INDEX] + _raw_lift;
			*_sv = rov->_midpoint[VERTICAL_RIGHT_INDEX] + _raw_lift;
		}

	}

	if (_headingHoldEnabled)
	{

		// Calculate heading error
		_hdg_Error = rov->mpu_campass - _tgt_Hdg;

		if (_hdg_Error > 180)
		{
			_hdg_Error = _hdg_Error - 360;
		}

		if (_hdg_Error < -179)
		{
			_hdg_Error = _hdg_Error + 360;
		}

		// Run error accumulator (integrator)
		//_hdg_Error_Integral = _hdg_Error_Integral + _hdg_Error;

		// Calculator motor outputs
		_raw_yaw = -1 * _hdg_Error * _heading_loop_gain;

		// raw_Left = raw_Left - (_hdg_Error_Integral / integral_Divisor);
		// raw_Right = raw_Right + (_hdg_Error_Integral / integral_Divisor);

		// Constrain and output to motors

		_raw_yaw = sl_constrain(_raw_yaw, -50, 50);

		if (sl_abs(_hdg_Error) > _heading_deadband){
			*_p = rov->_midpoint[PORT_INDEX] + _raw_yaw;
			*_s = rov->_midpoint[STARBORD_INDEX] - _raw_yaw;
		}
	}
}
