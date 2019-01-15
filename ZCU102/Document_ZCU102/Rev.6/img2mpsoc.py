#! /usr/bin/env python
from __future__ import print_function
import os
import shutil
import sys
import socket
import select
from contextlib import closing
import cv2
import numpy as np
import glob
import copy
import argparse

import threading

def ishow(img,title='movie'):
	cv2.imshow(title,img)
	demoState.pause()
def ishow_splited_img(splited_img,go_shape=(768,1280,3),pr_shape=(256,256,3)):
	Nh,Nw,_ = np.ceil(go_shape/np.float32(pr_shape)).astype(np.int)
	for x in range(0,Nw):
		for y in range(0,Nh):
			ishow(splited_img[y,x],'loc='+str((x,y)))
def delw(): cv2.destroyAllWindows()

def prod2int(shape):return int(np.prod(shape))

def split_img(img,go_shape=(768,1280,3),pr_shape=(256,256,3)):
	assert img.shape[0]<=go_shape[0],'not support img height too big'
	assert img.shape[1]<=go_shape[1],'not support img width too big'
	assert img.shape[2]==go_shape[2],'not support img or output shape'
	img_outA= np.zeros(np.prod(go_shape)).astype(np.uint32).reshape(go_shape)
	#1st copy matrix into output area
	img_outA[:img.shape[0],:img.shape[1],:] = img[:img.shape[0],:img.shape[1],:]
	#2nd copy matrix with 5dimentions
	h,w,_   = pr_shape
	Nh,Nw,_ = np.ceil(go_shape/np.float32(pr_shape)).astype(np.int)
	img_outL=[ [ list(img_outA[h*Y:h*(Y+1),h*X:h*(X+1),:]) for X in range(0,Nw) ] for Y in range(0,Nh) ]
	return np.asarray(img_outL).astype(np.uint8)

class boxes(object):
	#def __init__(self,limit=15):
	def __init__(self,limit=4):
		self.clear()
		self._fullN=limit
	def clear(self):
		self._boxes=[]
		self._usedN=0
	def append(self,reply):
		if len(self._boxes)==self._fullN:return False
		self._usedN+=1
		self._boxes.append(reply)
		if len(self._boxes)==self._fullN:return False
		return True
	def __getitem__(self,idx):
		assert idx <= self._usedN-1,'index out of range'
		return self._boxes[idx]
	def	__iter__(self):
		self._idx=0
		return self
	def next(self):
		if self._usedN == self._idx: raise StopIteration()
		self._idx+=1
		return self._boxes[self._idx-1]

# detinfo format spec:
# list(partNo, kindNo, kindNo, kindNo, kindNo)
#      where kindNo means object No.
#      and list len must be fix as 5Bytes, for windows bug...
#      kindNo 0 means padding of list less than 5Bytes, so ignored.
#
detinfo_length_byte = 6
col_B,col_G,col_R=((255,0,0),(0,255,0),(0,0,255))
idx2detinfo	=	['ignored','car','pedestrian','sign','light','nothing']
detinfo_col=[(0,0,0),col_R,col_B,col_G,(255,255,0),(255,255,255),(0,255,255),(255,0,255),(0,0,0)]
def draw_box2img(img,boxes,pr_shape=(256,256,3)):
	Nh,Nw,_ = np.ceil(img.shape/np.float32(pr_shape)).astype(np.int)
	imgB = copy.deepcopy(img)
	for b in boxes:
		if len(b)<=1: continue
		partNo=b[0]
		lt_y=int(partNo/Nw)*pr_shape[0]
		lt_x=(partNo%Nw)*pr_shape[1]
		rb_y=lt_y+pr_shape[0]
		rb_x=lt_x+pr_shape[1]
		for e in b[1:]:
			if e==0:continue	# No.0 is padding of reply record, so ignored
			C=detinfo_col[e]
			# print('e=',e,'C=',C)	#for DEBUG
			cv2.rectangle(imgB,(lt_x,lt_y),(rb_x,rb_y),C,2)
	return imgB

def img2string(img,encode=None):
	assert img.dtype == np.dtype(np.uint8),'image must be in numpy.uint8'
	img = img.transpose(2,0,1)	# transpose to be matched cnn recognizer
	string = img.tostring()
	if encode: return string.encode(encode)
	return string

def string2array1D(string,encode=None):
	if encode: string = string.decode(encode)
	return np.fromstring(string,dtype=np.uint8)

def detinfo2string(partNo,classes):
	assert isinstance(partNo,int) and isinstance(classes,list)
	return np.array([partNo]+classes,dtype=np.uint8).tostring()

def string2detinfo(string):
	assert isinstance(string,str)
	return list(np.fromstring(string,dtype=np.uint8))

def view_top4(box,img,title='Class-'):
	for i,answer in enumerate(box):
		if i==0 or answer<=1:continue	# ignore SubFrameNo(i==0). or answer 'car(1)' or 'ignore(0)'
		assert answer<len(idx2detinfo)
		windowname = title + str(idx2detinfo[answer])
		C = detinfo_col[answer]
		font = cv2.FONT_HERSHEY_PLAIN
		cv2.rectangle(img,(0,0),(255,255),C,3)
		cv2.putText(img,idx2detinfo[answer],(15,25),font,2,C)
		cv2.imshow(windowname,img)

def initWindows(title='Class-'):
	img = np.zeros(256*256*3,dtype=np.uint8).reshape(256,256,3)
	for k in range(2,len(idx2detinfo)):
		#windowname = title + str(idx2detinfo[k]+'('+str(k)+')')
		windowname = title + str(idx2detinfo[k])
		cv2.imshow(windowname,img)
	img = np.zeros(1280*768*3,dtype=np.uint8).reshape(768,1280,3)
	cv2.imshow('movie',img)
	cv2.waitKey(100)

def picture(filename,resize=(1280,768)):
	if len(glob.glob(filename))==0:print('not found',filename);return np.array([]).astype(np.uint8)
	img = cv2.imread(filename)
	assert isinstance(img,np.ndarray),filename
	if [img.shape[0],img.shape[1]] != [resize[1],resize[0]]:
		return cv2.resize(img,resize)
	return img

class pictures(object):
	def __init__(self,img,resize=(1280,768),pr_shape=(256,256,3),host='127.0.0.1',port=8888,encode=None):
		self.img =img
		self.host=host
		self.port=port
		self.pr_shape=pr_shape
		self.encode=encode
		self.x=self.y=0
		self.splited = split_img(img,go_shape=(resize[1],resize[0],3),pr_shape=self.pr_shape)
		self.aspect  = np.ceil([resize[0],resize[1],3]/np.float32(pr_shape)).astype(np.int)
		self.primitives = prod2int(self.aspect)
		self.dim_y, self.dim_x = self.splited.shape[:2]
	def set_TCP(host,port,encode=None):
		self.host=host
		self.port=port
		self.encode=encode
	def __iter__(self):
		self.x=self.y=0
		return self
	def next(self):
		if self.y >= self.dim_y:
			raise StopIteration
		else:
			img = self.splited[self.y][self.x]
		self.x+=1
		if self.x==self.dim_x:
			self.x=0
			self.y+=1
		return img
	def save(self,frameNo,definfo,sub_img,directory='dataset'):
		for i,kind in enumerate(definfo):
			if i==0 or kind==0:continue
			directory += '/'+str(kind)
			if len(glob.glob(directory))==0:
				os.makedirs(directory)
			newNo= 0
			while True:
				filename = directory+'/'+str(frameNo)+'_'+str(newNo)+'.jpg'
				if len(glob.glob(filename))==0:break
				newNo+=1
			cv2.imwrite(filename,sub_img)
	def view(self,title='pictures'):
		for f in self:
			demoState.pause()
			if demoState.acquire_quit():break
			ishow(f)
	def send(self):
		demoState.pause()
		reset_cmd(demoState.SubFrameS,demoState.SubFrameE,self.host,self.port)
		SubFrameS=demoState.SubFrameS
		SubFrameE=demoState.SubFrameE
		b = boxes(SubFrameE-SubFrameS+1)
		boxes_vacancy=True
		global g_frameNo
		for frameNo, f in enumerate(self):
			if frameNo < SubFrameS or frameNo > SubFrameE:continue
			demoState.pause()
			demoState.engine_mode_cmd(self.host,self.port)
			if demoState.acquire_quit():break
			box = send_img2server(f,self.host,self.port,self.encode)
			definfo = string2detinfo(box)
			global socket_error_framNo
			if definfo[0] != frameNo:
				print('SubFrameNo. mismatch send=%d : recieved=%d' % (frameNo,definfo[0]))
			if definfo[0] == socket_error_framNo:
				definfo[0] = frameNo
			boxes_vacancy = b.append(definfo)
			view_top4(definfo,f,title='Class-')
			ximg = draw_box2img(self.img,b,self.pr_shape)	# For Full SubFrame Drawing
			if demoState.acquire_writing(): self.save(g_frameNo,definfo,f)
			cv2.imshow('movie',ximg)
			cv2.waitKey(1)
		if not boxes_vacancy:
			return draw_box2img(self.img,b,self.pr_shape)

class movie(object):
	def __init__(self,img_source=0,resize=(1280,768),pr_shape=(256,256,3),host='127.0.0.1',port=8888,encode=None):
		self._resize = resize
		self._host=host
		self._port=port
		self._encode=encode
		self._pr_shape = pr_shape
		self._cap = cv2.VideoCapture(img_source)
		if not self._cap.isOpened():print('can not open movie',img_source);return
		if isinstance(img_source,int):
			print('WebCam  Frame W:H  =',self._cap.get(3),':',self._cap.get(4))
			print('WebCam  SAT/HUE/EXP=',self._cap.get(12),'/',self._cap.get(13),'/',self._cap.get(15))
	def set_TCP(host,port,encode=None):
		self._host=host
		self._port=port
		self._encode=encode
	def __iter__(self):
		return self
	def next(self):
		if self._cap.isOpened():
			return self._cap.read()
		else:
			self._cap.release()
			raise StopIteration()
	def release(self):self._cap.release()
	def view(self,title='movie'):
		for r,f in self:
			demoState.pause()
			if demoState.acquire_quit():break
			if not r:self._cap.release();return
			f = cv2.resize(f,self._resize)
			cv2.imshow(title,f)
			if cv2.waitKey(30)==27:break
		self._cap.release()
	def save(self,directory='./dataset'):
		j = 0
		for r,f in self:
			os.makedirs(directory+'/'+str(j))
			if not r:self._cap.release();print('can not get video frame');return
			f = cv2.resize(f,self._resize)
			for i,f in enumerate(pictures(f,self._resize,self._pr_shape)):
				cv2.imwrite(directory+'/'+str(j)+'/'+str(i)+'.jpg',f)
			j+=1
	def send(self,title='movie'):
		global g_frameNo
		g_frameNo=0
		for r,f in self:
			g_frameNo+=1
			if (g_frameNo % demoState.SkipFrame)!=0:continue
			if not r:self._cap.release();print('can not get video frame');return
			f = cv2.resize(f,self._resize)
			f = pictures(f,self._resize,self._pr_shape,self._host,self._port,self._encode).send()
			if demoState.acquire_quit():break
			cv2.imshow(title,f)
			if cv2.waitKey(1)==27:break
		self._cap.release()

def in2out():
	global socket_ioerror
	args = get_args()
	with demoState.lock:
		demoState.SubFrameS = args.subfrm[0]
		demoState.SubFrameE = args.subfrm[1]
		demoState.SkipFrame = args.skip
	if args.picture:
		filename = args.picture
		if len(glob.glob(filename))>0:
			img = picture(filename,args.resize)
			if args.transfer:
				img = pictures(img,args.resize,args.primitive,args.ip,args.port,args.encode).send()
				if socket_ioerror > 0:
					print('TC/IP socket ioerror statistics=',socket_ioerror)
					socket_ioerror=0
			else:
				ishow(img)
			if demoState.acquire_quit():return args
	elif args.movie:
		filename = args.movie
		if len(glob.glob(filename))>0:
			moviefile = movie(filename,args.resize,args.primitive,args.ip,args.port,args.encode)
			if args.transfer:
				moviefile.send()
				if socket_ioerror > 0:
					print('TC/IP socket ioerror statistics=',socket_ioerror)
					socket_ioerror=0
			else:
				moviefile.view()
			if demoState.acquire_quit:return args
	elif args.camera:
		camera = int(args.cameraNo)
		webcam = movie(camera,args.resize,args.primitive,args.ip,args.port,args.encode)
		webcam.view()
		if demoState.acquire_quit:return args
	return args

def save_dataset():
	args = get_args()
	# rename and mkdir new
	if len(glob.glob(args.dataset_dir))==1:
		if len(glob.glob(args.dataset_dir+'1'))==1:
			shutil.rmtree(args.dataset_dir+'1')
		shutil.move(args.dataset_dir,args.dataset_dir+'1')
	os.makedirs(args.dataset_dir)

	if args.picture:
		filename = args.picture
		j=0
		if len(glob.glob(filename))>0:
			img = picture(filename,args.resize)
			os.makedirs(args.dataset_dir+'/'+str(j))
			for i,f in enumerate(pictures(img,args.resize,args.primitive)):
				cv2.imwrite(args.dataset_dir+'/'+str(j)+'/'+str(i)+'.jpg',f)
			j+=1
	elif args.movie:
		filename = args.movie
		if len(glob.glob(filename))>0:
			moviefile = movie(filename,args.resize,args.primitive)
			moviefile.save(args.dataset_dir)
	elif args.camera:
		print('video camera saving not support, total picture size is huge')

def get_args():
	parser = argparse.ArgumentParser()
	group1 = parser.add_mutually_exclusive_group()
	group1.add_argument('-c','--camera'     ,action="store_true")
	group1.add_argument('-m','--movie'      ,nargs=1)
	group1.add_argument('-p','--picture'    ,nargs=1)
	group2 = parser.add_mutually_exclusive_group()
	group2.add_argument('-t','--transfer'   ,action="store_true")
	group2.add_argument('-s','--server_mode',action="store_true")
	group2.add_argument('-D','--dataset_out',action="store_true")
	parser.add_argument('-C','--cameraNo'   ,nargs=1,default=0,type=int)
	parser.add_argument('-d','--dataset_dir',nargs='?',default='./dataset',type=str)
	parser.add_argument('-I','--ip'         ,nargs=1,default='127.0.0.1',type=str)
	parser.add_argument('-P','--port'       ,nargs=1,default=8888,type=int)
	parser.add_argument('-B','--buffer_size',nargs=1,default=256*256*3,type=int)
	parser.add_argument('-v','--view_server',action="store_true")
	parser.add_argument('--encode'          ,nargs=1,default=None)
	parser.add_argument('--resize'          ,nargs=2,default=[1280,768],type=int)
	parser.add_argument('--primitive'       ,nargs=2,default=[256,256],type=int)
	parser.add_argument('--skip'            ,nargs=1,default=15,type=int)
	parser.add_argument('--subfrm'          ,nargs=2,default=[0,9],type=int)
	args = parser.parse_args()
	if isinstance(args.movie,list):
		args.movie     = args.movie[0]
	if isinstance(args.picture,list):
		args.picture   = args.picture[0]
	if isinstance(args.cameraNo,list):
		args.cameraNo   = args.cameraNo[0]
	if args.resize:
		args.resize  = tuple(args.resize)
	if isinstance(args.primitive,list) and len(args.primitive)==2:
		args.primitive = tuple(args.primitive+[3])
	if isinstance(args.subfrm,list) and len(args.subfrm)==2:
		args.subfrm  = tuple(args.subfrm)
	if isinstance(args.ip,list):
		args.ip      = str(args.ip[0])
	if isinstance(args.port,list):
		args.port    = args.port[0]
	if isinstance(args.skip,list):
		args.skip    = args.skip[0]
	if isinstance(args.buffer_size,list):
		args.buffer_size = args.buffer_size[0]
	if args.encode is not None:
		args.encode  = str(args.encode[0])
	if isinstance(args.ip,list):
		args.dataset_dir = str(args.dataset_dir[0])
	assert args.resize[1]>=args.primitive[0]
	assert args.resize[0]>=args.primitive[1]
	assert args.resize[1]% args.primitive[0] == 0,'resize must be multiple of primitive axis 0'
	assert args.resize[0]% args.primitive[1] == 0,'resize must be multiple of primitive axis 1'
	# append new member
	args.aspect      = tuple(np.ceil((args.resize[1],args.resize[0])/np.float32(args.primitive[:2])).astype(np.int))
	args.primitive_size = prod2int(args.primitive)
	args.primitives     = prod2int(args.aspect)
	return args

socket_ioerror=0
socket_error_framNo=255
def send_string2server(string,host='127.0.0.1',port=8888):
	global socket_ioerror
	msg = ''
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	#sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
	#sock.settimeout(1)
	try:
		with closing(sock):
			sock.connect((host, port))
			sock.send(string)
			sock.settimeout(60)	#for DEBUG
			box = sock.recv(detinfo_length_byte)
			# print('box=',np.fromstring(box,dtype=np.uint8))	#for DEBUG
			sock.close()
	except IOError:
		if socket_ioerror%120==0:print('TCP/IP socket IOError',socket_ioerror)
		box = detinfo2string(socket_error_framNo,[0,0,0,0])
		sock.close()
		socket_ioerror+=1
	return box

def send_img2server(img,host='127.0.0.1',port=8888,encode=None):
	string = img2string(img,encode=None)
	return send_string2server(string,host,port)

stat_comm_string=np.array([0xA,0x3]*10,dtype=np.uint8).tostring()
def stat(host='127.0.0.1',port=8888):
	send_string2server(stat_comm_string)
def change_EM_cmd(mode,host='127.0.0.1',port=8888):
	CMD_CHANGE_ENGINE_MODE = 0xC
	pad = 256*256*3 - 20 - 2
	cmd=np.array([0xA,0x5]*10+[CMD_CHANGE_ENGINE_MODE,mode]+[0x0]*pad,dtype=np.uint8).tostring()
	return send_string2server(cmd,host,port)
def reset_cmd(begin,end,host='127.0.0.1',port=8888):
	CMD_RESET_SUBFRAME_RNG = 0xB
	pad = 256*256*3 - 20 - 3
	cmd=np.array([0xA,0x5]*10+[CMD_RESET_SUBFRAME_RNG,begin,end]+[0x0]*pad,dtype=np.uint8).tostring()
	return send_string2server(cmd,host,port)

def server_for_test():
	args = get_args()
	host = args.ip
	port = args.port
	bufsize = args.primitive_size
	bufsize = args.buffer_size
	print('server_for_test:ip=',host,':',port,'buffer=',bufsize,'Byte')
	backlog = 10
	TCP_transfer_flag=True
	pictureNo = 0
	stat_pict = 0;stat_Byte = 0

	server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server_sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)

	with closing(server_sock):
		server_sock.bind((host, port))
		server_sock.listen(backlog)
		img_string=''
		while True:
			conn, address = server_sock.accept()
			while True:
				msg = conn.recv(bufsize)
				stat_Byte+=len(msg)
				if msg==stat_comm_string:
					print('socke_ioerror =',socket_ioerror)
					print('recieved pict =',stat_pict)
					print('recieved Byte =',stat_Byte)
					print('Byte/picture  =',stat_Byte/int(stat_pict+(stat_pict==0)))
					break
				elif TCP_transfer_flag:
					img_string += msg
					if len(img_string) >= args.primitive_size:
						img = string2array1D(img_string,args.encode).reshape(args.primitive)
						if args.view_server:
							cv2.imshow('view in server',img)
						cv2.waitKey(1)	# instead of server side process time
						img_string=''
						classes = list(np.random.permutation(np.random.permutation(6)[0]))
						conn.send(detinfo2string(pictureNo,classes))
						pictureNo+=1
						if pictureNo == args.primitives:pictureNo = 0
						stat_pict+=1
						break
			conn.close()
	return

class DemoState:
	def __init__(self):
		print("init DemoState")
		# Member variables
		self.lock= threading.RLock()
		self.run =  False
		self.quit = False
		self.loop = False
		self.gray = False
		self.SubFrameS = 0
		self.SubFrameE = 9
		self.SkipFrame =15
		self.writing   = False
		self.engine    = 0
	def check(self):
		print("run  cont   loop  gray %d-%d/%d" % (self.SubFrameS,self.SubFrameE,self.SkipFrame))
		print(self.run,not self.run,self.loop,self.gray)
	def pause(self):
		while True:
			with self.lock:
				if self.run:return
				cv2.waitKey(1)
	def acquire_quit(self):
		with self.lock: ans = self.quit
		return ans
	def acquire_writing(self):
		with self.lock: ans = self.writing
		return ans
	def reqChangeEngine(self,engineMode):
		if engineMode > 0:
			with self.lock: self.engine = engineMode
	def engine_mode_cmd(self,host,port):
		with self.lock:
			if self.engine > 0:
				change_EM_cmd(self.engine,host,port)
				self.engine = 0
	def in2out(self):
		self.in2out=threading.Thread(target=in2out)
		self.in2out.start()
		return self.in2out
	def in2out_join(self):
		self.in2out.join()

demoState = DemoState()
def checkCommand():
	while True:
		print("< q(uit) p(ause) c(ontinue) l(oop) g(ray) >")
		print(
			"<",demoState.quit,"",not demoState.run,"  ",demoState.run,"    ",demoState.loop,"",demoState.gray," >",
			"subfrm=(from-to)/skip=",demoState.SubFrameS,"-",demoState.SubFrameE,"/",demoState.SkipFrame
		)
		key=raw_input()
		with demoState.lock:
			if key=='q':
				print("quit")
				demoState.loop = False
				demoState.run  = True
				demoState.quit = True
				sys.exit(0)
			elif key=='p':
				demoState.run = False
				print("pause")
			elif key=='c':
				demoState.run = True
				print("continue")
			elif key=='l':
				demoState.loop = not demoState.loop
				print('loop=',demoState.loop)
			elif key=='g':
				demoState.gray = not demoState.gray
				print('gray=',demoState.gray)
			elif key=='r':
				demoState.reqChangeEngine(1)
				print('engine=',demoState.engine)
			elif key=='n':
				demoState.reqChangeEngine(2)
				print('engine=',demoState.engine)
			elif key=='won':
				demoState.writing = True
				print('writing subframe on',demoState.writing)
			elif key=='woff':
				demoState.writing = False
				print('writing subframe off',demoState.writing)
			elif key[0:8]=='subfrm=(':
				if len(key)==14:
					demoState.SubFrameS = int(key[8:10])
					demoState.SubFrameE = int(key[11:13])
					print('valid subframe from %d to %d (1-15)' % (demoState.SubFrameS,demoState.SubFrameE))
				else:
					print('syntax ignore subframes setting : ignore=(xx,xx)')
			elif key[0:5]=='skip=':
				if len(key)==7:
					demoState.SkipFrame=int(key[5:8])
					print('skip frames =',demoState.SkipFrame)
				else:
					print('syntax skip frames setting : skip=xx')

if __name__ == '__main__':
	args = get_args()
	np.random.seed(10)
	if args.server_mode and (not args.movie and not args.picture and not args.camera):
		print('host-ip        =',args.ip)
		print('port           =',args.port)
		print('buffer_size    =',args.buffer_size)
		print('primitive      =',args.primitive)
		print('primitive_size =',args.primitive_size)
		print('primitives     =',args.primitives)
		print('encode         =',args.encode)
		server_for_test()
	else:
		print('host-ip        =',args.ip)
		print('port           =',args.port)
		print('buffer_size    =',args.buffer_size)
		print('primitive      =',args.primitive)
		print('primitive_size =',args.primitive_size)
		print('primitives     =',args.primitives)
		print('encode         =',args.encode)
		print('resize         =',args.resize)
		print('aspect         =',args.aspect)
		print('subfrm         =',args.subfrm)
		print('skip           =',args.skip)
		if args.dataset_out:
			print('pictures or movie will be splited and saved into',args.dataset_dir)
			save_dataset()	#A.I.
		else:
			initWindows()
			cv2.waitKey(10)
			th=threading.Thread(target=checkCommand)
			th.start()
			in2out()
			while demoState.loop:
				print('retry->')
				in2out()
			th.join()

