#!/usr/bin/env python
#-----------------------------------------------------------------------------
# File:        TB3Ddisplay.py
# Description: TB 2014 simple 3D display
# Created:     30-May-2014 Harrison B. Prosper & Sam Bein
#-----------------------------------------------------------------------------
import sys, os, re
from ROOT import *
from string import lower, replace, strip, split, joinfields, find
from array import array
import numpy 
#------------------------------------------------------------------------------
x_y_scale_factor = 20
class Display3D:

	def __init__(self, page):

		self.page = page
		
		# Coordinates:
		# +z to the right, in direction of beam
		# +x into the page
		# +y vertically upwards
		
		self.geometry = TEveElementList("Testbeam 2014 Geometry")
        
		# Wire Chambers
		# 1/2 thickness is Z
		dx1 =  32 * x_y_scale_factor # mm - half length in x at lower z surface
		dx2 =  32 * x_y_scale_factor# mm - half length in x at upper z surface
		dy  =  32 * x_y_scale_factor# mm - half length in y
		dz  =   20.0 # mm - half length in z
		z_WC2  = 3476.6 # mm - position of  Wire chamber 2
		transparency = 0
		
		self.wc1 = TEveGeoShape('WC 1')
		self.wc1.SetShape( TGeoTrd1(dx1, dx2, dy, dz) )
		self.wc1.SetMainColor(kCyan)
		self.wc1.SetMainTransparency(transparency)
		self.geometry.AddElement(self.wc1)

		self.wc2 = TEveGeoShape('WC 2')
		self.wc2.SetShape( TGeoTrd1(dx1, dx2, dy, dz) )
		self.wc2.SetMainColor(kMagenta)
		self.wc2.SetMainTransparency(transparency)
		self.wc2.RefMainTrans().SetPos(0,0, z_WC2)
		self.geometry.AddElement(self.wc2)

		dx1 =  28/4 * x_y_scale_factor # mm - half length in x of cell
		dx2 =  28/4 * x_y_scale_factor # mm - same thing
		dy  =  28/4 * x_y_scale_factor # mm - half length in y of cell
		dz  = 113.5  # mm - half length in z (of shashlik)
		z0  = 3476.6+533.4+253

		# Draw shashlik modules
		color = [kYellow, kBlue]
		kk = -1
		step = 2*dx1
		xmin =-2*step
		ymin =-2*step
		self.module = []
		for ii in xrange(4):
			x = xmin + (ii+0.5)*step
			for jj in xrange(4):
				kk += 1
				y = ymin + (jj+0.5)*step
				icolor = kk % 2
				if jj == 3: kk += 1
				
				self.module.append(TEveGeoShape('Shashlik%d%d' % (ii, jj)))
				self.module[-1].SetShape( TGeoTrd1(dx1, dx2, dy, dz) )
				self.module[-1].SetMainColor(color[icolor])
				self.module[-1].SetMainTransparency(transparency)
				self.module[-1].RefMainTrans().SetPos(x, y, z0)
				self.geometry.AddElement(self.module[-1])

		gEve.AddElement(self.geometry)

	def __del__(self):
		pass
	
	def Show(self):
		gEve.Redraw3D(kTRUE)
		refPos = [5.0,60.0,100.0]
		refPos = numpy.asarray(refPos, dtype=numpy.float64)
		print "my crazy array is ", refPos

		#gEve.GetDefaultGeometry()
		v = gEve.GetDefaultGLViewer() 
		v.SetPerspectiveCamera (2,31,100,refPos,10,15)
		#v.SetResetCamerasOnUpdate(kFALSE)
		gEve.Redraw3D(kTRUE)
		
	#----------------------------------------------------------------------
	# Draw hits
	#----------------------------------------------------------------------
	def Draw(self, event, accumulate=False, stealthmode=False):	
		from random import uniform
		from random import randint

		elements = self.page.elements
		elements.DestroyElements()

		# mimic WC 1 hits
		N = randint(2, 10)
		wc1hits = TEvePointSet(N)
		wc1hits.SetName("marker1")
		wc1hits.SetMarkerColor(kGreen)
		wc1hits.SetMarkerStyle(1)
		wc1hits.SetMarkerSize(1.5)
		for ii in xrange(N):
			x = uniform(-32/4* x_y_scale_factor, 32/4* x_y_scale_factor)
			y = uniform(-32/4* x_y_scale_factor, 32/4* x_y_scale_factor)
			z = 0.0
			wc1hits.SetPoint(ii, x, y, z)
		elements.AddElement(wc1hits)

		# mimic WC 2 hits
		N = randint(2, 10)
		wc2hits = TEvePointSet(N)
		wc2hits.SetName("marker1")
		wc2hits.SetMarkerColor(kGreen)
		wc2hits.SetMarkerStyle(2)
		wc2hits.SetMarkerSize(1.5)
		for ii in xrange(N):
			x = uniform(-32/4* x_y_scale_factor, 32/4* x_y_scale_factor)
			y = uniform(-32/4* x_y_scale_factor, 32/4* x_y_scale_factor)
			z = 3476.6
			wc2hits.SetPoint(ii, x, y, z)
		elements.AddElement(wc2hits)

		self.Show()
