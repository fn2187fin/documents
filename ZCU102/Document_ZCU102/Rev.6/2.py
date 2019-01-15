# -*- coding:utf-8 -*-
import sys
from PyQt4.QtGui  import *
from PyQt4.QtCore import *
import threading
import time

class DemoState(object):
	def __init__(self):
		super(DemoState,self).__init__()
		self.run=False
		self.quit=False
		self.SubFrameS = 1
		self.SubFrameE = 9
		self.SkipFrame = 15
		self.MPSoCMode = 1	# 1:reduced 2:normal else pass
		self.lock=threading.RLock()
	def pause(self):
		while True:
			with self.lock:
				if self.run:
					break
	def acquire_quit(self):
		with self.lock:
			ans=self.quit
		return ans
demoState=DemoState()

def in2out():
	count=0
	while True:
		demoState.pause()
		count+=1
		lim = 100000*demoState.SubFrameS
		if (count % lim)==0:
			print "in2out",count
		if demoState.acquire_quit():
			break

class PyQtWidget(QWidget):

	def __init__(self,app):
		super(PyQtWidget, self).__init__()
		self.run  = False
		self.quit = False
		self.app=app
		self.initUi()

	def initUi(self):
		grid = QGridLayout()
		grid.setSpacing(10)

		self.lblSpace = QLabel('_____________')

		self.cmbMode = QComboBox(self)
		self.lblSubFrameS = QLabel('begin')
		self.lblSubFrameE = QLabel('end')
		self.lblSkipFrame = QLabel('skip')
		self.cmbSubFrameS = QComboBox(self)
		self.cmbSubFrameE = QComboBox(self)
		self.cmbSkipFrame = QComboBox(self)
		self.cmbMode.addItems((u'Reduced',u'Normal',u'Pass'))
		self.cmbSubFrameS.addItems(('0','1','2'))
		self.cmbSubFrameE.addItems(('9','11','12'))
		self.cmbSkipFrame.addItems(('10','20','30'))
		self.cmbMode.activated[str].connect(self.changeMode)
		self.cmbSubFrameS.activated[str].connect(self.changeSubFrameS)
		self.cmbSubFrameE.activated[str].connect(self.changeSubFrameE)
		self.cmbSkipFrame.activated[str].connect(self.changeSkipFrame)

		self.chkLoop = QCheckBox('Loop',self)

		# Buttons
		self.btnExec  = QPushButton('continue',self)
		self.btnClear = QPushButton('pause',self)
		self.btnQuit  = QPushButton('quit',self)
		self.btnExec.clicked.connect(self.GUIcontinue)
		self.btnClear.clicked.connect(self.GUIpause)
		self.btnQuit.clicked.connect(self.GUIquit)

		# Progress
		self.pbarL= QLabel('Progress')
		self.pbar = QProgressBar(self)
		self.pbar.setRange(0, 100)

		# Grid for only QWidget
		grid.addWidget(self.btnExec,      0,0)
		grid.addWidget(self.btnClear,     0,1)
		grid.addWidget(self.btnQuit,      0,2)
		grid.addWidget(self.lblSpace,     0,3)
		grid.addWidget(self.lblSpace,     0,4)
		grid.addWidget(self.lblSpace,     0,5)
		grid.addWidget(self.lblSpace,     0,6)
		grid.addWidget(self.cmbMode,      1,0)
		grid.addWidget(self.lblSubFrameS, 1,1)
		grid.addWidget(self.cmbSubFrameS, 1,2)
		grid.addWidget(self.lblSubFrameE, 1,3)
		grid.addWidget(self.cmbSubFrameE, 1,4)
		grid.addWidget(self.lblSkipFrame, 1,5)
		grid.addWidget(self.cmbSkipFrame, 1,6)
		grid.addWidget(self.chkLoop,      2,0)
		grid.addWidget(self.pbarL,        3,0)
		grid.addWidget(self.pbar,         3,1)
		self.setLayout(grid)

	def GUIquit(self, value):
		with demoState.lock:
			demoState.run  = True
			demoState.quit = True
		self.app.quit()
	def changeMode(self, value):
		print value
		with demoState.lock:
			if value == 'Reduced':
				demoState.MPSoCMode = 1
			elif value == 'Normal':
				demoState.MPSoCMode = 2
			else:
				demoState.MPSoCMode = 0
	def changeSubFrameS(self, value):
		with demoState.lock:
			demoState.SubFrameS = int(value)
	def changeSubFrameE(self, value):
		with demoState.lock:
			demoState.SubFrameE = int(value)
	def changeSkipFrame(self, value):
		with demoState.lock:
			demoState.SkipFrame = int(value)
	def GUIcontinue(self, value):
		with demoState.lock:
			demoState.run = True
	def GUIpause(self, value):
		with demoState.lock:
			demoState.run = False

def GUIx():
	app = QApplication(sys.argv)
	th=threading.Thread(target=in2out)
	th.start()
	gui = PyQtWidget(app)
	gui.show()
	sys.exit(app.exec_())
	th.join()

if __name__ == '__main__':
	GUIx()
