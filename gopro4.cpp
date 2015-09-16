#include "gopro4.h"
#include "slconfig.h"
#include "slserver.h"
#ifdef SLSERVER_LINUX
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif
#include <string.h>
#include <errno.h>
#include "global.h"


#ifdef SLSERVER_LINUX
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif
#endif

#ifdef USE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
};
#endif

#define MAX_BUFFER_LEN 4096
#include <stdlib.h>

struct gopro4_control {
	const char* method;
	const char* url;
};

//https://github.com/KonradIT/goprowifihack/blob/master/HERO4.md
static const gopro4_control _gopro4[]= {
	{"gopro_on","http://10.5.5.9/gp/gpControl/setting/9/0"},
	{"gopro_off","http://10.5.5.9/gp/gpControl/setting/9/1"},
	{"gopro_ev_2","http://10.5.5.9/gp/gpControl/setting/26/0"},
	{"gopro_ev_1_5","http://10.5.5.9/gp/gpControl/setting/26/1"},
	{"gopro_ev_1","http://10.5.5.9/gp/gpControl/setting/26/2"},
	{"gopro_ev_0_5","http://10.5.5.9/gp/gpControl/setting/26/3"},
	{"gopro_ev_0","http://10.5.5.9/gp/gpControl/setting/26/4"},						
	{"gopro_ev_neg_0_5","http://10.5.5.9/gp/gpControl/setting/26/5"},	
	{"gopro_ev_neg_1","http://10.5.5.9/gp/gpControl/setting/26/6"},
	{"gopro_ev_neg_1_5","http://10.5.5.9/gp/gpControl/setting/26/7"},
	{"gopro_ev_neg_2","http://10.5.5.9/gp/gpControl/setting/26/8"},	
	{"gopro_white_balance_auto","http://10.5.5.9/gp/gpControl/setting/11/0"},			
	{"gopro_white_balance_3000k","http://10.5.5.9/gp/gpControl/setting/11/1"},	
	{"gopro_white_balance_5500k","http://10.5.5.9/gp/gpControl/setting/11/2"},	
	{"gopro_white_balance_6500k","http://10.5.5.9/gp/gpControl/setting/11/3"},	
	{"gopro_white_balance_native","http://10.5.5.9/gp/gpControl/setting/11/4"},	
	{"gopro_color_gopro","http://10.5.5.9/gp/gpControl/setting/12/1"},	
	{"gopro_color_flat","http://10.5.5.9/gp/gpControl/setting/12/2"},	
	{"gopro_sharpness_high","http://10.5.5.9/gp/gpControl/setting/14/0"},	
	{"gopro_sharpness_med","http://10.5.5.9/gp/gpControl/setting/14/1"},	
	{"gopro_sharpness_low","http://10.5.5.9/gp/gpControl/setting/14/2"},	
	{"gopro_iso_6400","http://10.5.5.9/gp/gpControl/setting/13/0"},	
	{"gopro_iso_1600","http://10.5.5.9/gp/gpControl/setting/13/1"},
	{"gopro_iso_400","http://10.5.5.9/gp/gpControl/setting/13/2"},
	{"gopro_primary_modes_video","http://10.5.5.9/gp/gpControl/command/mode?p=0"},
	{"gopro_primary_modes_photo","http://10.5.5.9/gp/gpControl/command/mode?p=1"},
	{"gopro_primary_modes_multishot","http://10.5.5.9/gp/gpControl/command/mode?p=2"},
	{"gopro_secondary_modes_video","http://10.5.5.9/gp/gpControl/setting/68/0"},
	{"gopro_secondary_modes_timelapse_video","http://10.5.5.9/gp/gpControl/setting/68/1"},
	{"gopro_secondary_modes_video_add_photo","http://10.5.5.9/gp/gpControl/setting/68/2"},
	{"gopro_secondary_modes_looping","http://10.5.5.9/gp/gpControl/setting/68/3"},
	{"gopro_secondary_modes_single","http://10.5.5.9/gp/gpControl/setting/69/0"},
	{"gopro_secondary_modes_continuous","http://10.5.5.9/gp/gpControl/setting/69/1"},
	{"gopro_secondary_modes_night","http://10.5.5.9/gp/gpControl/setting/69/2"},
	{"gopro_secondary_modes_burst","http://10.5.5.9/gp/gpControl/setting/70/0"},
	{"gopro_secondary_modes_timelapse","http://10.5.5.9/gp/gpControl/setting/70/1"},
	{"gopro_secondary_modes_nightlapse","http://10.5.5.9/gp/gpControl/setting/70/2"},
	{"gopro_power_off","http://10.5.5.9/gp/gpControl/command/system/sleep"},
	{"gopro_frame_rate_120fps","http://10.5.5.9/gp/gpControl/setting/3/0"},
	{"gopro_frame_rate_90fps","http://10.5.5.9/gp/gpControl/setting/3/3"},
	{"gopro_frame_rate_60fps","http://10.5.5.9/gp/gpControl/setting/3/5"},
	{"gopro_frame_rate_48fps","http://10.5.5.9/gp/gpControl/setting/3/7"},
	{"gopro_frame_rate_30fps","http://10.5.5.9/gp/gpControl/setting/3/8"},
	{"gopro_frame_rate_24fps","http://10.5.5.9/gp/gpControl/setting/3/10"},
	{"gopro_resolutions_4k","http://10.5.5.9/gp/gpControl/setting/2/1"},
	{"gopro_resolutions_4k_superView","http://10.5.5.9/gp/gpControl/setting/2/2"},
	{"gopro_resolutions_2_7k","http://10.5.5.9/gp/gpControl/setting/2/4"},
	{"gopro_resolutions_2_7k_superView","http://10.5.5.9/gp/gpControl/setting/2/5"},
	{"gopro_resolutions_2_7k_4:3","http://10.5.5.9/gp/gpControl/setting/2/6"},
	{"gopro_resolutions_1440p","http://10.5.5.9/gp/gpControl/setting/2/7"},
	{"gopro_resolutions_1080p_superView","http://10.5.5.9/gp/gpControl/setting/2/8"},
	{"gopro_resolutions_1080p","http://10.5.5.9/gp/gpControl/setting/2/9"},
	{"gopro_resolutions_960p","http://10.5.5.9/gp/gpControl/setting/2/10"},
	{"gopro_resolutions_720p_superView","http://10.5.5.9/gp/gpControl/setting/2/11"},
	{"gopro_resolutions_720p","http://10.5.5.9/gp/gpControl/setting/2/12"},
	{"gopro_resolutions_wvga","http://10.5.5.9/gp/gpControl/setting/2/13"},
	{"gopro_exposure_time_for_nightphoto_auto","http://10.5.5.9/gp/gpControl/setting/19/0"},
	{"gopro_exposure_time_for_nightphoto_2","http://10.5.5.9/gp/gpControl/setting/19/1"},
	{"gopro_exposure_time_for_nightphoto_5","http://10.5.5.9/gp/gpControl/setting/19/2"},
	{"gopro_exposure_time_for_nightphoto_10","http://10.5.5.9/gp/gpControl/setting/19/3"},
	{"gopro_exposure_time_for_nightphoto_15","http://10.5.5.9/gp/gpControl/setting/19/4"},
	{"gopro_exposure_time_for_nightphoto_20","http://10.5.5.9/gp/gpControl/setting/19/5"},
	{"gopro_exposure_time_for_nightphoto_30","http://10.5.5.9/gp/gpControl/setting/19/6"},
	{"gopro_exposure_time_for_nightlapse_auto","http://10.5.5.9/gp/gpControl/setting/31/0"},
	{"gopro_exposure_time_for_nightlapse_2","http://10.5.5.9/gp/gpControl/setting/31/1"},
	{"gopro_exposure_time_for_nightlapse_5","http://10.5.5.9/gp/gpControl/setting/31/2"},
	{"gopro_exposure_time_for_nightlapse_10","http://10.5.5.9/gp/gpControl/setting/31/3"},
	{"gopro_exposure_time_for_nightlapse_15","http://10.5.5.9/gp/gpControl/setting/31/4"},
	{"gopro_exposure_time_for_nightlapse_20","http://10.5.5.9/gp/gpControl/setting/31/5"},
	{"gopro_exposure_time_for_nightlapse_30","http://10.5.5.9/gp/gpControl/setting/31/6"},
	{"gopro_photo_resolution_12mp_wide","http://10.5.5.9/gp/gpControl/setting/17/0"},
	{"gopro_photo_resolution_7mp_wide","http://10.5.5.9/gp/gpControl/setting/17/1"},
	{"gopro_photo_resolution_7mp_wedi","http://10.5.5.9/gp/gpControl/setting/17/2"},
	{"gopro_photo_resolution_5mp_wide","http://10.5.5.9/gp/gpControl/setting/17/3"},
	{"gopro_filed_of view_wide","http://10.5.5.9/gp/gpControl/setting/4/0"},
	{"gopro_filed_of view_medium","http://10.5.5.9/gp/gpControl/setting/4/1"},
	{"gopro_filed_of view_narrow","http://10.5.5.9/gp/gpControl/setting/4/2"},
	{"gopro_low_light_on","http://10.5.5.9/gp/gpControl/setting/8/1"},
	{"gopro_low_light_off","http://10.5.5.9/gp/gpControl/setting/8/0"},
	{"gopro_timelapse_interval_0_5","http://10.5.5.9/gp/gpControl/setting/5/0"},
	{"gopro_timelapse_interval_1","http://10.5.5.9/gp/gpControl/setting/5/1"},
	{"gopro_timelapse_interval_2","http://10.5.5.9/gp/gpControl/setting/5/2"},
	{"gopro_timelapse_interval_5","http://10.5.5.9/gp/gpControl/setting/5/3"},
	{"gopro_timelapse_interval_10","http://10.5.5.9/gp/gpControl/setting/5/4"},
	{"gopro_timelapse_interval_30","http://10.5.5.9/gp/gpControl/setting/5/5"},
	{"gopro_timelapse_interval_60","http://10.5.5.9/gp/gpControl/setting/5/6"},
	{"gopro_continues_photo_rate_3","http://10.5.5.9/gp/gpControl/setting/18/0"},
	{"gopro_timelapse_interval_5","http://10.5.5.9/gp/gpControl/setting/18/1"},
	{"gopro_timelapse_interval_10","http://10.5.5.9/gp/gpControl/setting/18/2"},
	{"gopro_video_looping_duration_max","http://10.5.5.9/gp/gpControl/setting/6/0"},
	{"gopro_video_looping_duration_5min","http://10.5.5.9/gp/gpControl/setting/6/1"},
	{"gopro_video_looping_duration_20min","http://10.5.5.9/gp/gpControl/setting/6/2"},
	{"gopro_video_looping_duration_60min","http://10.5.5.9/gp/gpControl/setting/6/3"},
	{"gopro_video_looping_duration_120min","http://10.5.5.9/gp/gpControl/setting/6/4"},
	{"gopro_video+photo_interval_5","http://10.5.5.9/gp/gpControl/setting/7/1"},
	{"gopro_video+photo_interval_10","http://10.5.5.9/gp/gpControl/setting/7/2"},
	{"gopro_video+photo_interval_30","http://10.5.5.9/gp/gpControl/setting/7/3"},
	{"gopro_video+photo_interval_60min","http://10.5.5.9/gp/gpControl/setting/7/4"},
	{"gopro_shutter_trigger","http://10.5.5.9/gp/gpControl/command/shutter?p=1"},
	{"gopro_stop_(video/timelapse)","http://10.5.5.9/gp/gpControl/command/shutter?p=0"},
	{"gopro_start_streaming","http://10.5.5.9/gp/gpControl/execute?p1=gpStream&c1=start"},
	{"gopro_restart_streaming","http://10.5.5.9/gp/gpControl/execute?p1=gpStream&c1=restart"},
	{"gopro_stop_streaming","http://10.5.5.9/gp/gpControl/execute?p1=gpStream&c1=stop"}
};

int gopro4::ffmpeg_init = 0;

gopro4::gopro4(int trans_t):_conn(4096,16){
	_recv = INVALID_SOCKET;
	_send = INVALID_SOCKET;
	_heartbeat_sender = INVALID_SOCKET; 
	_recv_buffer = new char[MAX_BUFFER_LEN];
	_trans_type = trans_t;
	
#ifdef SLSERVER_LINUX
	_wifi = NULL;
#endif

	if (ffmpeg_init <= 0)
	{
		av_register_all();
		avformat_network_init();
		ffmpeg_init ++ ;
	}
	_vd_on_func = NULL;
	_vd_on_data = NULL;
	_vd_off_func = NULL;
	_vd_off_data = NULL;
	
}

gopro4::~gopro4() {
	delete[] _recv_buffer;
}

bool gopro4::init() {
	SOCKADDR_IN servAddr;
	//unsigned long ul = 1;
	if(_trans_type == GOPRO4_TRANSFER_PORT) {
		_recv = socket(AF_INET,SOCK_DGRAM,0);
		if(_recv == INVALID_SOCKET) {
			LOGOUT("***ERROR*** Create SOCKET ERROR\n");
			return false;
		}
		/*
		if (ioctlsocket(_recv, FIONBIO, &ul) == SOCKET_ERROR) {
			LOGOUT("***ERROR*** SET SOCKET NONBLOCK ERROR\n");
			return false;
		}
		*/
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(short(GOPRO4_UDP_PORT));
	#ifdef SLSERVER_WIN32
		servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	#else
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	#endif
		if(bind(_recv,(SOCKADDR*)&servAddr,sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
			LOGOUT("***ERROR*** Bind ADDRESS ERROR\n");
			return false;
		}
		
		_send = socket(AF_INET,SOCK_DGRAM,0);
		if(_send == INVALID_SOCKET) {
			LOGOUT("***ERROR*** Create SOCKET ERRor\n");
			return false;
		}
		
	}
	
	_heartbeat_sender = socket(AF_INET, SOCK_DGRAM, 0);
	if (_heartbeat_sender == INVALID_SOCKET) {
		LOGOUT("***ERROR*** Create SOCKET ERRor\n");
		return false;
	}

	_heartbeat_addr.sin_family = AF_INET;
	_heartbeat_addr.sin_port = htons(8554);
#ifdef SLSERVER_WIN32
	_heartbeat_addr.sin_addr.S_un.S_addr = inet_addr(GOPRO4_IP);
#else
	_heartbeat_addr.sin_addr.s_addr = inet_addr(GOPRO4_IP);
#endif
	_heart_len = sizeof(_heartbeat_addr);
	
	sprintf_s(_beat,"_GPHD_:%u:%u:%d:%6f\n",0, 0, 2, 0.f);
	_beat_len = strlen(_beat);
	_beat_running = false;
	_is_start = false;
	_is_working = false;

#ifdef SLSERVER_LINUX
	if(!slglobal.pConfig->getString("GOPRO_SSID",_ssid))
		strcpy(_ssid,"gopro12345");
	
	if(!slglobal.pConfig->getString("GOPRO_PSK",_psk))
		strcpy(_psk,"12345678");

	if(!slglobal.pConfig->getString("WPA_CTRL_IFACE",_wifi_ctrl_iface))
		strcpy(_wifi_ctrl_iface,"/var/run/wpa_supplicant/wlan0");
	
	_wifi = new wifi_manager;
	
	if(_wifi->init(_wifi_ctrl_iface) == false) {
		LOGOUT("***ERROR*** cannot open wifi ctrl iface,please sure you run with sudo\n");
		
		delete _wifi;
		_wifi = NULL;
		
		return false;
	}
	_isWifiConnect = false;
	_wifi->onEvent(WPA_EVENT_CONNECTED,gopro4::onWifiConnected,this);
	_wifi->onEvent(WPA_EVENT_DISCONNECTED,gopro4::onWifiDisconnected,this);		

#endif	
	
	return true;
}

#ifdef SLSERVER_LINUX

#if 0
bool gopro4::start2() {
	wifi_status t_status;
	bool ret = false;
	memset(&t_status,0,sizeof(wifi_status));
	
	do {
		if(_wifi == NULL)
			break;
		
		if(_wifi->getWifiStatus(&t_status) == false) {
			LOGOUT("***ERROR*** wifi interface get wifi status failed\n");
			break;
		}
		
		if(strcmp(t_status.ssid,_ssid) == 0) {
			ret = true;
			_isWifiConnect = true;
			this->start();
		}else{
			_wifi->onEvent(WPA_EVENT_CONNECTED,gopro4::onWifiConnected,this);
			_wifi->onEvent(WPA_EVENT_DISCONNECTED,gopro4::onWifiDisconnected,this);
			
			if(_wifi->connectWifi(_ssid) == false) {
				LOGOUT("***INFO*** cannot connect wifi,try to set\n");
				if(_wifi->addWifiNetwork(_ssid,_psk) == false)
					break;
				ret = _wifi->connectWifi(_ssid);
			}else
				ret = true;
		}
	}while(0);	
	
	return ret;
}
#endif

void gopro4::onWifiConnected(int level,const char* msg,void* data) {
	gopro4* _gopro4 = (gopro4*)data;
	wifi_status t_status;
	memset(&t_status,0,sizeof(wifi_status));
	
	if(_gopro4->_wifi->getWifiStatus(&t_status) == false) {
		LOGOUT("***ERROR*** wifi interface get wifi status failed\n");
		return;
	}
	
	if(t_status.ssid[0] != 0 && !_gopro4->_isWifiConnect) {
		char buf[32];
		char buf2[128];
		
		get_file_from_path(_gopro4->_wifi_ctrl_iface,buf);
		
		sprintf(buf2,"dhclient %s",buf);
		
		LOGOUT("***INFO*** %s\n",buf2);
		system(buf2);
		
		_gopro4->_isWifiConnect = true;
		
		_gopro4->start();

		
		sprintf(buf2,"{\"name\":\"wifi_event_connected\",\"args\":\"%s\"}",t_status.ssid);
		slglobal.server->broadcast(9,buf2);
		
		sprintf(buf2,"{\"name\":\"wifi_status\",\"args\":[{\"ssid\":\"%s\",\"signal\":0.8}]}",t_status.ssid);
		slglobal.server->broadcast(9,buf2);
	}
}

void gopro4::onWifiDisconnected(int level,const char* msg,void* data) {
	gopro4* _gopro4 = (gopro4*)data;
	LOGOUT("***ERROR*** wifi disconnect\n");
	
	_gopro4->_isWifiConnect = false;
	
	slglobal.server->broadcast(9,"{\"name\":\"wifi_event_disconnected\",\"args\":\"[]\"}");
	slglobal.server->broadcast(9,"{\"name\":\"wifi_status\",\"args\":\"[]\"}");
}

void gopro4::broadcastWifiStatus() {
	
	wifi_status t_status;
	memset(&t_status,0,sizeof(wifi_status));
	
	if(_wifi->getWifiStatus(&t_status) == false) {
		LOGOUT("***ERROR*** wifi interface get wifi status failed\n");
		return;
	}
	
	if(t_status.ssid[0] != 0 && !_isWifiConnect) {
		char pBUf[128];
		
		sprintf(pBUf,"{\"name\":\"wifi_status\",\"args\":[{\"ssid\":\"%s\",\"signal\":0.8}]}",t_status.ssid);
		slglobal.server->broadcast(9,pBUf);
	}
}

#endif

#ifdef SLSERVER_WIN32
int gettimeofday(struct timeval *tp, void *tzp);
#endif

void* gopro4::heartbeat(void *user) {
	gopro4* pointer = (gopro4*)user;
	struct timeval now;
	struct timespec outtime;
	
	pthread_mutex_lock(&pointer->_beat_int_mutex);
	
	while(pointer->_beat_running) {
		
		if(sendto(pointer->_heartbeat_sender,pointer->_beat,pointer->_beat_len,0,(SOCKADDR*)&pointer->_heartbeat_addr,pointer->_heart_len) == SOCKET_ERROR) {
			LOGOUT("***ERROR*** heatbeat send error\n");
			pointer->_beat_running = false;
		}else{
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 2;
			outtime.tv_nsec = now.tv_usec * 1000 + 500000000;
			pthread_cond_timedwait(&pointer->_beat_int_cond, &pointer->_beat_int_mutex, &outtime);	
		}
	}
	pthread_mutex_unlock(&pointer->_beat_int_mutex);
	
	return NULL;
}

void* gopro4::transfer(void* user) {
	gopro4* pointer = (gopro4*)user;
	int nRecv = 0;
	SOCKADDR_IN addr_client;
#ifdef SLSERVER_WIN32
	int nlen = sizeof(SOCKADDR_IN);
#else
	socklen_t nlen = sizeof(SOCKADDR_IN);
#endif
	while(pointer->_trans_running) { 
		if((nRecv = recvfrom(pointer->_recv,pointer->_recv_buffer,MAX_BUFFER_LEN, 0,(SOCKADDR*)&addr_client,&nlen)) == SOCKET_ERROR) {
#ifdef SLSERVER_WIN32
			if(WSAGetLastError() == WSAEWOULDBLOCK) 
				continue;
			else {
				LOGOUT("***ERROR *** get packet error\n");
				break;
			}
#endif
		}
		
		pthread_mutex_lock(&pointer->_client_mutex);
		for(list<Client>::node* p = pointer->_clients.begin(); p != pointer->_clients.end(); p = p->next) {
			if(sendto(pointer->_send,pointer->_recv_buffer,nRecv,0,(SOCKADDR*)&p->element.sock,sizeof(p->element.sock)) == SOCKET_ERROR) {
				LOGOUT("***ERROR *** Client send error\n");
			}
		}
		pthread_mutex_unlock(&pointer->_client_mutex);
#ifdef SLSERVER_WIN32
//		Sleep(1);
#endif
	}

	return NULL;
}

void* gopro4::transfer_stream(void* data) {
#ifdef USE_FFMPEG
	gopro4* pointer = (gopro4*)data;
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	int videoindex;
	int i;
	AVFrame	*pFrame;		
	int ret, got_picture,got_output;
	AVPacket *packet;
	AVInputFormat *ifmt;
	AVPacket *pktOut;
	AVCodecContext* pJpegCodecCtx;
	AVCodec* pJpegCodec;
	
	ifmt = av_find_input_format("mpegts");
	pFormatCtx = avformat_alloc_context();
	
	if (avformat_open_input(&pFormatCtx, "udp://10.5.5.9:8554" ,ifmt, NULL) != 0) {
		LOGOUT("***ERROR*** Couldn't open input stream.\n");
		return NULL;
	}

	
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		LOGOUT("Couldn't find stream information.\n");
		return NULL;
	}
	
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		LOGOUT("Couldn't find a video stream.\n");
		return NULL;
	}
	
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		LOGOUT("Codec not found.\n");
		return NULL;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		LOGOUT("Could not open codec.\n");
		return NULL;
	}
	
	pJpegCodec = avcodec_find_encoder(CODEC_ID_MJPEG);
	
	if (!pJpegCodec) {
        LOGOUT("could not find mjpeg encode\n");
        return NULL;
    }
	
	pJpegCodecCtx = avcodec_alloc_context3(pJpegCodec);
    if (!pJpegCodecCtx) {
        LOGOUT("Could not allocate video codec context\n");
        return NULL;
    }
	
	pJpegCodecCtx->bit_rate = pCodecCtx->bit_rate;
	pJpegCodecCtx->width = pCodecCtx->width;
	pJpegCodecCtx->height = pCodecCtx->height;
	pJpegCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;
	pJpegCodecCtx->codec_id = CODEC_ID_MJPEG;
	pJpegCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pJpegCodecCtx->time_base.num = pCodecCtx->time_base.num;
	pJpegCodecCtx->time_base.den = pCodecCtx->time_base.den;
	
	 /* open it */
    if (avcodec_open2(pJpegCodecCtx, pJpegCodec, NULL) < 0) {
        LOGOUT("Could not open codec\n");
        return NULL;
    }
	
	pFrame = av_frame_alloc();

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	pktOut = (AVPacket *)av_malloc(sizeof(AVPacket));
	
	//Output Information-----------------------------
	LOGOUT("File Information---------------------\n");
	av_dump_format(pFormatCtx, 0, NULL, 0);
	LOGOUT("-------------------------------------------------\n");

	slglobal.is_stream_running = true;
	slglobal.frame_alloc_size = 512 * 1024;
	if (slglobal.frame == NULL)
		slglobal.frame = (unsigned char*)malloc(slglobal.frame_alloc_size);

	if(pointer->_vd_on_func != NULL)
		(*pointer->_vd_on_func)(pointer->_vd_on_data);
	
	while(pointer->_trans_running) {
		
		if (av_read_frame(pFormatCtx, packet) >= 0)
		{
			if (packet->stream_index == videoindex)
			{
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if (ret < 0)
				{
					LOGOUT("Decode Error.\n");
					break;
				}
				if (got_picture)
				{
					av_init_packet(pktOut);
					pktOut->data = NULL;
					pktOut->size = 0;
					
					ret = avcodec_encode_video2(pJpegCodecCtx, pktOut, pFrame, &got_output);
					if (ret < 0) {
						LOGOUT("Error encoding frame\n");
						break;
					}

					if (got_output) {
#ifdef SLSERVER_LINUX
						pthread_mutex_lock(&slglobal.frame_lock);
#else
						LOCK_GLOBAL_FRAME_LOCK;
#endif
						if(pktOut->size > slglobal.frame_alloc_size) {
			
							if(slglobal.frame != NULL)
								free(slglobal.frame);
							
							slglobal.frame = (unsigned char*)malloc(pktOut->size + 1024);
							slglobal.frame_alloc_size = pktOut->size + 1024;
						}
						slglobal.frame_size = pktOut->size;
						slglobal.frame_count++;

						memcpy(slglobal.frame,pktOut->data,pktOut->size);
#ifdef SLSERVER_LINUX
						pthread_mutex_unlock(&slglobal.frame_lock);
#else
						UNLOCK_GLOBAL_FRAME_LOCK;
#endif
					}
					av_free_packet(pktOut);
				}
			}	
		} else {
#ifdef SLSERVER_WIN32
			Sleep(1);
#else
			usleep(1);
#endif
		}
	}
	slglobal.is_stream_running = false;
	av_free(packet);
	av_free(pktOut);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	avcodec_close(pJpegCodecCtx);
	avcodec_free_context(&pJpegCodecCtx);
#endif
	LOGOUT("*********tranfer return***************\n");
	
	return NULL;
}

bool gopro4::start() {

	if (_is_start)
		return true;

	_is_start = true;
	_beat_running = true;
	_trans_running = true;
	
	pthread_mutex_init(&_beat_int_mutex, NULL);
	pthread_cond_init(&_beat_int_cond, NULL);
	pthread_mutex_init(&_client_mutex,NULL);

	if (0 != pthread_create(&_beat_thread, NULL, gopro4::heartbeat, (void*)this)) {
		printf("***ERROR*** gopro4 when create pthread heart beat thread,%d\n", errno);
		return false;
	}

	if(_trans_type == GOPRO4_TRANSFER_PORT) {
		if (0 != pthread_create(&_trans_thread, NULL, gopro4::transfer, (void*)this)) {
			printf("***ERROR*** gopro4 when create pthread transfer thread,%d\n", errno);
			return false;
		}	
		
	} else {
		if (0 != pthread_create(&_trans_thread, NULL, gopro4::transfer_stream, (void*)this)) {
			printf("***ERROR*** gopro4 when create pthread transfer thread,%d\n", errno);
			return false;
		}
	}
	
	runCommand("gopro_restart_streaming");

	return true;
}

void gopro4::stop() {
	if (!_is_start)
		return;

	_is_start = false;
	_beat_running = false;
	_trans_running = false;
	
	pthread_mutex_lock(&_beat_int_mutex);
	pthread_cond_signal(&_beat_int_cond);
	pthread_mutex_unlock(&_beat_int_mutex);

	pthread_join(_trans_thread, NULL);
	pthread_join(_beat_thread, NULL);
	
	
	pthread_mutex_destroy(&_beat_int_mutex);
	pthread_mutex_destroy(&_client_mutex);
}

int gopro4::addClient(int uid,const char* ip) {
	if(_trans_type != GOPRO4_TRANSFER_PORT)
		return -1;
	
	//fix add client more than once 
	pthread_mutex_lock(&_client_mutex);
	for(list<Client>::node* p = _clients.begin(); p != _clients.end(); p = p->next) {
		if( p->element.uid == uid) {
			return _clients.getSize();
		}
	}
	pthread_mutex_unlock(&_client_mutex);
	
	Client c(uid,ip);
	pthread_mutex_lock(&_client_mutex);
	_clients.push_back(c);
	pthread_mutex_unlock(&_client_mutex);

	return _clients.getSize();
}

int gopro4::runCommand(const char* cmd) {
	const gopro4_control* t = NULL;
	HttpStream* outs = NULL;
	for (int i = 0; i < sizeof(_gopro4) / sizeof(gopro4_control); i++) {
		if (strcmp(_gopro4[i].method, cmd) == 0) {
			t = &_gopro4[i];
			break;
		}
	}
	if (t) {
		if (_conn.open(t->url) == false) {
			LOGOUT("***ERROR*** run gopro4 cmd:%s\n", t->method);
			return -1;
		}
		//outs = _conn.getHttpOutputStream();

	}
	return 1;
}

void gopro4::removeClient(int uid) {
	for(list<Client>::node* p = _clients.begin(); p != _clients.end(); p = p->next) {
		if(p->element.uid == uid) {
			pthread_mutex_lock(&_client_mutex);
			_clients.erase(p);
			pthread_mutex_unlock(&_client_mutex);
			break;
		}
	}
}

bool gopro4::gopro_wol(const char* ip, unsigned short port) {
	unsigned char _mac[6] = {0};
	SOCKET sock = INVALID_SOCKET;

	LOGOUT("***INFO*** gopro_wol\n");

/*
	if (_init_mac) {
		memcpy(_mac, _smac, 6);
	}
	else {
		if (SlServer::getMacAddr(ip, _mac) == false)
			return false;
	}
*/
#ifdef SLSERVER_WIN32
	if (SlServer::getMacAddr(ip, _mac) == false) {
		return false;
	}
#else
	wifi_status t_status;
	memset(&t_status,0,sizeof(wifi_status));
	
	if(_wifi->getWifiStatus(&t_status) == false) {
		LOGOUT("***ERROR*** wifi interface get wifi status failed at gopro_wol\n");
		return false;
	}
	
	memcpy(_mac,t_status.bssid,6);
#endif
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("Socket create error: %d\n", errno);
		return false;
	}

	int bOptVal = 1;
	int iOptLen = sizeof(int);
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, iOptLen) == SOCKET_ERROR)
	{
		fprintf(stderr, "setsockopt error: %d\n", errno);
		closesocket(sock);

		return false;
	}

	sockaddr_in to;
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = inet_addr(ip);

	char magicpacket[102];
	memset(magicpacket, 0xff, 102);
	memcpy(magicpacket + 6, _mac, 6);
	memcpy(magicpacket + 12, _mac, 6);
	memcpy(magicpacket + 18, _mac + 6, 12);
	memcpy(magicpacket + 30, _mac + 6, 24);
	memcpy(magicpacket + 54, _mac + 6, 48);

	if (sendto(sock, (const char *)magicpacket, 102, 0, (const struct sockaddr *)&to, sizeof(to)) == SOCKET_ERROR) {
		printf("Magic packet send error: %d", errno);
	}
		
	printf("***INFO*** Magic packet send!\n");

	closesocket(sock);
	return true;
}

bool gopro4::test_is_work()
{
	struct sockaddr_in _sockaddr;
	
#ifdef SLSERVER_LINUX
	if(_isWifiConnect == false)
		return false;
#endif

	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET) {
		LOGOUT("***ERROR*** can not open socket\n");
		return false;
	}
	
	_sockaddr.sin_family = AF_INET;
	_sockaddr.sin_addr.s_addr = inet_addr(GOPRO4_IP);
	_sockaddr.sin_port = htons(80);

#ifdef SLSERVER_LINUX
	unsigned long ul = 1;
	timeval tm;
	fd_set set;
	ioctl(_sock, FIONBIO, &ul); 
	int error=-1, len;
	len = sizeof(int);
	bool ret = false;
	
	if( connect(_sock, (struct sockaddr *)&_sockaddr, sizeof(_sockaddr)) == -1)
	{
		tm.tv_sec = 1;
		tm.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(_sock, &set);
		if( select(_sock+1, NULL, &set, NULL, &tm) > 0)
		{
			getsockopt(_sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
			if(error == 0) 
				ret = true;
			else 
				ret = false;
		} else 
			ret = false;
	}else 
		ret = true;
	
	closesocket(_sock);
	return ret;
#else
	if (SOCKET_ERROR == connect(_sock, (struct sockaddr*)&_sockaddr, sizeof(_sockaddr))) {
		LOGOUT("***ERROR*** connect GOPRO4 error\n");
		return false;
	}

	shutdown(_sock, 0x02);
	closesocket(_sock);

	return true;
#endif
}

#if 0
bool gopro4::_init_mac = false;

unsigned char gopro4::_smac[6];
#endif

gopro4::Client::Client() {
	uid = -1;
	memset(&sock,0,sizeof(sock));
}

gopro4::Client::Client(int u,const char* ip) {
	uid = u;
	sock.sin_family = AF_INET;
	sock.sin_port = htons(GOPRO4_MAP_PORT);
#ifdef SLSERVER_WIN32
	sock.sin_addr.S_un.S_addr = inet_addr(ip);
#else
	sock.sin_addr.s_addr = inet_addr(ip);
#endif
}

void gopro4::setVideoOn(video_handler _f,void* data) {
	_vd_on_func = _f;
	_vd_on_data = data;
}

void gopro4::setVideoOff(video_handler _f,void* data) {
	_vd_off_func = _f;
	_vd_off_data = data;
}

#ifdef SLSERVER_LINUX
rapidjson::StringBuffer& gopro4::wifi_scan_results(rapidjson::StringBuffer& p) {
	rapidjson::Document d;
	char _buf[64] = {0};
	d.SetObject();

	d.AddMember("name",rapidjson::Value().SetString("wifi_scan_results"),d.GetAllocator());

	rapidjson::Value args(rapidjson::kArrayType);

	list<wifi_scan*> _wifi_scans;

	if(_wifi->getAvaiableWifi(&_wifi_scans) == true) {
		for(list<wifi_scan*>::node* _p = _wifi_scans.begin(); _p != _wifi_scans.end(); _p = _p -> next) {
			rapidjson::Value _l(rapidjson::kObjectType);

			_l.AddMember("ssid",rapidjson::Value().SetString(_p->element->ssid,d.GetAllocator()),d.GetAllocator());
			_l.AddMember("frequency",rapidjson::Value().SetInt(_p->element->frequency),d.GetAllocator());
			_l.AddMember("signal",rapidjson::Value().SetInt(_p->element->signal_level),d.GetAllocator());
			sprintf(_buf,"%2x:%2x:%2x:%2x:%2x:%2x",_p->element->bssid[0]&0xff,_p->element->bssid[1]&0xff,_p->element->bssid[2]&0xff,_p->element->bssid[3]&0xff,_p->element->bssid[4]&0xff,_p->element->bssid[5]&0xff);
			_l.AddMember("mac",rapidjson::Value().SetString(_buf,d.GetAllocator()),d.GetAllocator());

			args.PushBack(_l,d.GetAllocator());
		}
		clearWifiList(&_wifi_scans);
	}

	d.AddMember("args",args,d.GetAllocator());
	rapidjson::Writer<rapidjson::StringBuffer> writer(p);

	d.Accept(writer);
	
	return p;
}

bool gopro4::connectWifi(const char* ssid,const char* psk) {
	wifi_status t_status;
	bool ret = false;
	memset(&t_status,0,sizeof(wifi_status));
	
	do {
		if(_wifi == NULL)
			break;
		
		if(_wifi->getWifiStatus(&t_status) == false) {
			LOGOUT("***ERROR*** wifi interface get wifi status failed\n");
			break;
		}
		
		if(strcmp(t_status.ssid,ssid) == 0) {
			
			ret = true;
			_isWifiConnect = true;
			this->start();
			
		}else{
			
			if(_wifi->connectWifi(ssid) == false) {
				LOGOUT("***INFO*** cannot connect wifi,try to set\n");
				
				if(psk == NULL) {
					if(strcmp(ssid,_ssid) == 0)
						psk = _psk;
				} else {
					LOGOUT("***INFO*** cannot connect %s,please set passwd\n",ssid);
				}
				
				if(_wifi->addWifiNetwork(ssid,psk) == false)
					break;
				
				ret = _wifi->connectWifi(ssid);
			}else
				ret = true;
		}
	}while(0);	
	
	return ret;
}

#endif
