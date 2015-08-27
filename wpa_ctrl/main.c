#include <unistd.h>
#include "wpa_ctrl.h"
#include <stdio.h>

int main(int args,char** argv ) {
	struct wpa_ctrl* _ctrl;
	char reply[1024] = {0};
	size_t reply_len = 1024;
	_ctrl = wpa_ctrl_open("/var/run/wpa_supplicant/wlan0");

	if(_ctrl == NULL) {
		printf("open wpa_ctrl error\n");
		return 1;
	}
	
	if(wpa_ctrl_request(_ctrl,"SCAN",4,reply,&reply_len,NULL)) {
		wpa_ctrl_close(_ctrl);
		return 1;
	}
	printf("scan %s\n",reply);
	
	reply_len = 1024;
	if(wpa_ctrl_request(_ctrl,"SCAN_RESULTS",12,reply,&reply_len,NULL)) {
		wpa_ctrl_close(_ctrl);
		return 1;
	}
	printf("scan_results %s\n",reply);

	wpa_ctrl_close(_ctrl);
	return 0;
}
