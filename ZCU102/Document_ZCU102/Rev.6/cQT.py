import numpy as np
import cv2

import sys
from PyQt4 import QtGui, QtCore
 
idx2detinfo=['ignored','car','pedestrian','sign','light','nothing']

class panel(QtGui.QWidget):
 
	def __init__(self):
		super(panel, self).__init__()

		# Widgets
		self.cmbLabelS = QtGui.QLabel(self)
		self.cmbLabelS.setText('SubFrame Begin')
		self.cmbSubFrameS = QtGui.QComboBox(self)
		self.cmbSubFrameS.addItems(('1','2','3','4','5','6'))
		self.cmbLabelE = QtGui.QLabel(self)
		self.cmbLabelE.setText('End')
		self.cmbSubFrameE = QtGui.QComboBox(self)
		self.cmbSubFrameE.addItems(('15','14','13','12','11','10','9','8','7'))
		self.cmbLabelF = QtGui.QLabel(self)
		self.cmbLabelF.setText('Skip Frames')
		self.cmbSkipFrame = QtGui.QComboBox(self)
		self.cmbSkipFrame.addItems(('1','2','3','4','5','6','7','8','9','10','15','20','30','60','90'))
 
		self.chk_loop = QtGui.QCheckBox('infinit loop',self)
		self.chk_gray = QtGui.QCheckBox('Gray Scale',self)
 
		self.btnExec = QtGui.QPushButton('continue',self)
		self.btnExec.clicked.connect(self.doContinue)
 
		self.btnClear = QtGui.QPushButton('pause',self)
		self.btnClear.clicked.connect(self.doPause)

		# Grid Layout
		grid1 = QtGui.QGridLayout()
		grid1.setSpacing(10)
		grid1.addWidget(self.btnExec,		0,0)
		grid1.addWidget(self.btnClear,		0,1)
		grid1.addWidget(self.chk_loop,		2,0)
		grid1.addWidget(self.chk_gray,		2,1)
		grid1.addWidget(self.cmbLabelS,		3,0)
		grid1.addWidget(self.cmbSubFrameS,	3,1)
		grid1.addWidget(self.cmbLabelE,		3,2)
		grid1.addWidget(self.cmbSubFrameE,	3,3)
		grid1.addWidget(self.cmbLabelF,		3,4)
		grid1.addWidget(self.cmbSkipFrame,	3,5)
		self.setLayout(grid1)

		self.setWindowTitle('VIDEO Interface for CNN(HITACHI ULSI Systems, Co.,Ltd.)')
		self.show()
 
	def doContinue(self):
		global demoState
		print "Cont"
		with demoState.lock:
			print "get Cont"
			demoState.run  = True
			demoState.loop = self.chk_loop.isChecked()
			demoState.gray = self.chk_gray.isChecked()
			demoState.SubFrameS = int(self.cmbSubFrameS.currentText())
			demoState.SubFrameE = int(self.cmbSubFrameE.currentText())
			demoState.SkipFrame = int(self.cmbSkipFrame.currentText())
			demoState.check()
 
	def doPause(self, value):
		global demoState
		with demoState.lock:
			demoState.run  = False
			demoState.check()
 
def GUIx():
	g_panel = QtGui.QApplication(sys.argv)
	panelObj = panel()
	g_panel.exec_()

def main():
	global demoState
	args = get_args()
	print args.ip
	initWindows()
	demoState.in2out()
	# th=threading.Thread(target=in2out)
	# th.start()
	# while True:pass
	GUIx()
	demoState.quit=True
	demoState.run =True
	demoState.in2out_join()
	return
 
from img2mpsoc import *

if __name__ == '__main__':
	main()
