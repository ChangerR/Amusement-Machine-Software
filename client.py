#! /usr/bin/python
import socket
import time
import wx
import threading
import thread
import os
import subprocess

sock = None
sendbox = None
wnd = None
btn_connect = None
output = None
app = None
sche = None
sock_recv = None
lock = threading.RLock() 
isConnect = False
_play = None
class recviver(threading.Thread): #The timer class is derived from the class threading.Thread  
    def __init__(self, num):  
		threading.Thread.__init__(self)  
		self.thread_num = num  
		self.thread_stop = False 
		self.setDaemon(True)
   
    def run(self): #Overwrite run() method, put what you want the thread do here  
		while not self.thread_stop:  
			global sock
			s = sock.recv(256)
			print 'recv'+s
			lock.acquire()
			output.AppendText(s)
			lock.release()
		
    def stop(self):  
		self.thread_stop = True 


class timer(threading.Thread): #The timer class is derived from the class threading.Thread  
    def __init__(self, num, interval,func):  
		threading.Thread.__init__(self)  
		self.thread_num = num  
		self.interval = interval  
		self.thread_stop = False 
		self.func = func
		self.setDaemon(True)
   
    def run(self): #Overwrite run() method, put what you want the thread do here  
		while not self.thread_stop:  
			time.sleep(self.interval)  
			self.func()
		
    def stop(self):  
		self.thread_stop = True 

class player(threading.Thread): #The timer class is derived from the class threading.Thread  
	def __init__(self, num):  
		threading.Thread.__init__(self)  
		self.thread_num = num  
		self.setDaemon(True)
		self.child = None

	def run(self): #Overwrite run() method, put what you want the thread do here  
		#os.system('ffplay -fflags nobuffer -f:v mpegts -probesize 8192 udp://localhost:9000');
		self.child=subprocess.Popen (["ffplay","-fflags","nobuffer", "-f:v", "mpegts","-probesize","8192","udp://127.0.0.1:9000"],shell=True)
	
	def stop(self):
		self.child.kill()

def heatbeat():
	global sock
	sock.send('1:::\r\n')
	
def connect_handler(event):
	global sock
	global sock_recv
	global isConnect
	if not isConnect:
		sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		sock.connect(('192.168.46.136',8080))
		lock.acquire()
		output.AppendText('connect to server....\n')
		lock.release()
		sock.send('ROVA:::\r\n')
		sock.send('1:::\r\n')
		global sche
		sche = timer(1,45,heatbeat)
		sche.start()
		sock_recv = recviver(2)
		sock_recv.start()
		isConnect = True
	
def send_handler(event):
	global sock
	s = sendbox.GetValue()+'\r\n'
	sock.send(s)
	
def disconnect_handler(event):
	global isConnect
	global sock
	global sock_recv
	if isConnect :
		sock.send('0:::\r\n')
		sock.shutdown(2)
		sock.close()
		sche.stop()
		sock_recv.stop()
		isConnect = False
		lock.acquire()
		output.AppendText('disconnect to server....\n')
		lock.release()
		sock = None

def play_handler(event):
	global _play
	_play = player(88);
	_play.start()
	#

def display_handler(event):
	global _play
	_play.stop()

def onClose(event):
	wx.Exit()
	
if __name__ == '__main__':
	app = wx.App()
	wnd = wx.Frame(None,title='ROV TEST',size=(640,480))
	wnd.Bind(wx.EVT_CLOSE,handler=onClose)
	panel = wx.Panel(wnd,-1,pos=(0,0))
	sendbox = wx.TextCtrl(panel,id=0,value='',pos=(40,40),size=(400,-1))
	btn_send = wx.Button(panel,id=3,label='send',pos=(460,40),size=(80,25))
	btn_send.Bind(wx.EVT_BUTTON,handler=send_handler)
	output = wx.TextCtrl(panel,id=4,value='',pos=(40,75),size=(540,300),style=wx.TE_MULTILINE|wx.TE_READONLY)
	btn_connect = wx.Button(panel,id=1,label='connect',pos=(80,400),size=(80,25))
	btn_connect.Bind(wx.EVT_BUTTON,handler=connect_handler)
	btn_disconnect = wx.Button(panel,label='disconnect',pos=(400,400),size=(80,25))
	btn_disconnect.Bind(wx.EVT_BUTTON,handler=disconnect_handler)
	btn_play = wx.Button(panel,label='play',pos=(180,400),size=(80,25))
	btn_play.Bind(wx.EVT_BUTTON,handler=play_handler)
	btn_display = wx.Button(panel,label='display',pos=(300,400),size=(80,25))
	btn_display.Bind(wx.EVT_BUTTON,handler=display_handler)
	wnd.Show()
	app.MainLoop()
	sche.stop()
	sock_recv.stop()
	_play.stop()



