#!/usr/bin/env python
#-----------------------------------------------------------------------------
# File:        TBEventDisplay.py
# Description: TB 2014 Event Display
# Created:     09-May-2014 Harrison B. Prosper & Sam Bein
#              18-May-2014 HBP - simplify
#              19-May-2014 HBP - fix rewind/forward
#              09-Jun-2014 HBP - streamline calling of displays
#              12-Jun-2014 HBP - fix bugs in heatmap and surface plots
#-----------------------------------------------------------------------------
import sys, os, re
from ROOT import *
from time import ctime, sleep
from string import lower, replace, strip, split, joinfields, find
from glob import glob
from array import array
from gui.utils import *
from gui.TBFileReader import TBFileReader
from gui.TBWaveFormPlots import TracePlot, SurfacePlot
from gui.TBShashlikFaces import ShashlikHeatmap
from gui.TBDisplay3D import Display3D
#------------------------------------------------------------------------------
WIDTH        = 1000            # Width of GUI in pixels
HEIGHT       =  500            # Height of GUI in pixels
VERSION      = \
"""
TBEventDisplay.py %s
Python            %s
Root              %s
""" % ('v1.0',
       platform.python_version(),
       gROOT.GetVersion())
#-----------------------------------------------------------------------------
MINDELAY = 1.0 # (seconds) minimum delay between event displays

# Help
HELP = \
"""
The time has come the walrus said
"""

ABOUT = \
"""
%s
\tTB 2014
\te-mail: harry@hep.fsu.edu
""" % VERSION

# read modes
R_REWIND  =-1
R_ONESHOT = 0
R_FORWARD = 1

DEBUG = 1
#-----------------------------------------------------------------------------
# (A) Root Graphical User Interfaces (GUI)
#
#   A Root GUI is a double tree of widgets (that is, graphical objects) with
#   which the user can interact in order to orchestrate a series of actions.
#   One tree describes the child-to-parent relationships, while the other
#   describes the parent-to-child relationships. The latter describes the
#   graphical layout of widgets. In the Root GUI system the two trees are not
#   isomorphic. For example, the child-to-parent relationship of a TGPopupMenu
#   is TGPopupMenu -> TGWindow, however, TGMenuBar -> TGPopupMenu is 
#   a typical parent-to-child relationship.
#
#   o A child-to-parent relationship is defined by the child widget when the
#     latter is created.
#
#   o A parent-to-child relationship, that is, a specific layout of a widget
#     within another, is defined by the parent after it has been created
#     using its AddFrame method in which the child is specified.
#
#   Each widget can emit one or more signals, usually triggered by some user
#   manipulation of, or action on, the widget. For example, clicking on a
#   tab of a  TGTab, that is, a notebook, will cause the notebook to emit
#   the signal "Selected(Int_t)" along with the identity of the selected tab.
#   Signals are connected to "Slots", that is, actions. This is how a user
#   GUI interaction can be made to yield one or more actions. Any signal can be
#   connected to any slot. Indeed, the relationship between signals and slots
#   can be many-many. In practice, a slot is modeled as a procedure such as
#   a method of the GUI class.
#  
#   In summary, a Root GUI is a (double) tree of widgets with which the user
#   can interact, whose signals---usually generated by user interactions---are
#   connected to slots, that is, actions modeled as methods of the GUI class.
#
# (B) This GUI
#
#   window                   (TGWindow)
#
#     main                      (TGMainFrame)
#
#       menuBar                    (TGMenuBar)
#         menuFile                    (TGPopupMenu)
#         menuEdit                    (TGPopupMenu)
#         menuEvent                   (TGPopupMenu)
#         menuHelp                    (TGPopupMenu)
#
#       vframe                     (TGVerticalFrame)
#         toolBar                    (TGToolBar)
#           nextButton                  (TGPictureButton)
#           previosButton               (TGPictureButton)
#
#         hframe                     (TGHorizontalFrame)
#           noteBook                    (TGTab)
#
#         statusBar                  (TGSTatusBar)
#-----------------------------------------------------------------------------
class TBEventDisplay:
    """
    gui = TBEventDisplay(title)
    """

    def __init__(self, title, filename=None, width=WIDTH, height=HEIGHT):

        # Initial directory for open file dialog
        self.openDir  = os.environ['PWD']
        self.filename = filename

        #-------------------------------------------------------------------
        # Create main frame
        #-------------------------------------------------------------------
        # Establish a connection between the main frame's "CloseWindow()"
        # signal and the GUI's "close" slot, modeled as a method.
        # When the main frame issues the signal CloseWindow() this
        # triggers a call to the close method of this class.
        #-------------------------------------------------------------------
        self.root = gClient.GetRoot()
        self.main = TGMainFrame(self.root, width, height)
        self.main.SetWindowName(title)
        self.main.SetCleanup(kDeepCleanup)
        self.connection = Connection(self.main, "CloseWindow()",
                         self,      "close")

        #-------------------------------------------------------------------
        # Create menu bar
        #-------------------------------------------------------------------
        self.menuBar = MenuBar(self, self.main)

        self.menuBar.Add('File',
                 [('&Open',  'openFile'),
                  ('&Close', 'closeFile'),
                  0,
                  ('E&xit',  'exit')])

        self.menuBar.Add('Edit',
                 [('&Undo',  'undo')])


        self.menuBar.Add('Event',
                 [('&Next',     'nextEvent'),
                  ('&Previous', 'previousEvent'),
                  ('&Goto',     'gotoEvent'),
                0,
                  ('Set ADC cut', 'setADCcut'),
                  ('Set delay',   'setDelay')])

        self.menuBar.Add('Help',
                 [('About', 'about'),
                  ('Usage', 'usage')])

        #-------------------------------------------------------------------
        # Add vertical frame to the main frame to contain toolbar, notebook
        # and status window
        #-------------------------------------------------------------------
        self.vframe = TGVerticalFrame(self.main, 1, 1)
        self.main.AddFrame(self.vframe, TOP_X_Y)

        #-------------------------------------------------------------------
        # Add horizontal frame to contain toolbar
        #-------------------------------------------------------------------
        self.toolBar = TGHorizontalFrame(self.vframe)
        self.vframe.AddFrame(self.toolBar, TOP_X)

        # Add picture buttons

        self.nextButton = PictureButton(self, self.toolBar,
                        picture='GoForward.gif',
                        method='nextEvent',
                        text='go to next event')

        self.forwardButton = PictureButton(self, self.toolBar,
                           picture='Forward.png',
                           method='forwardPlayer',
                           text='foward event player')

        self.stopButton = PictureButton(self, self.toolBar,
                        picture='Stop.png',
                        method='stopPlayer',
                        text='stop event player')

        self.rewindButton = PictureButton(self, self.toolBar,
                          picture='Rewind.png',
                          method='rewindPlayer',
                          text='rewind event player')

        self.previousButton = PictureButton(self, self.toolBar,
                            picture='GoBack.gif',
                            method='previousEvent',
                            text='go to previous event')

        self.akkumulateButton = CheckButton(self, self.toolBar,
                        hotstring='Accumulate',
                        method='toggleAccumulate',
                        text='Akkumulate')

        #-------------------------------------------------------------------
        # Add a notebook with multiple pages
        #-------------------------------------------------------------------
        #self.accumulate = True
        self.noteBook = NoteBook(self, self.vframe, 'setPage', width, height)
        # Add pages 
        self.display = {}
        for pageName, constructor in [('WF traces',     'TracePlot(canvas)'),
                          ('WF surface',    'SurfacePlot(canvas)'),
                          ('ADC heatmap',   'ShashlikHeatmap(canvas)'),
                          ('Wire chambers', 'WCPlanes(canvas)'),
                          ('3D display',    'Display3D(page)')]:
            self.noteBook.Add(pageName)
            self.noteBook.SetPage(pageName)
            page = self.noteBook.page
            canvas = page.canvas
            print '==> building display: %s' % pageName
            self.display[pageName] = eval(constructor)

        #-------------------------------------------------------------------
        # Create a status bar, divided into two parts
        #-------------------------------------------------------------------
        self.statusBar = TGStatusBar(self.vframe, 1, 1)
        status_parts = array('i')
        status_parts.append(23)
        status_parts.append(77)
        self.statusBar.SetParts(status_parts, len(status_parts))
        self.vframe.AddFrame(self.statusBar, TOP_X)

        # Initial state

        self.noteBook.SetPage('ADC heatmap')
        self.ADCcut  = 500
        self.nevents = 0
        self.eventNumber = -1
        self.DELAY  = int(1000*MINDELAY)
        self.mutex  = TMutex(kFALSE)
        self.timer  = TTimer()
        self.timerConnection = Connection(self.timer, 'Timeout()',
                                          self, 'managePlayer')
        self.DEBUG  = DEBUG
        self.DEBUG_COUNT = 0

        # Initialize layout        
        self.main.MapSubwindows()
        self.main.Resize()
        self.main.MapWindow()
	self.wcplanes = self.display['Wire chambers']	

        #DEBUG
        # to debug a display uncomment next line
        filename = "data/test.root"

        if filename != None: self.__openFile(filename)

        #DEBUG
        # to debug a display uncomment next two lines
        self.noteBook.SetPage('ADC heatmap')
        self.displayEvent()
        
    def __del__(self):
        pass


    def debug(self, message):
        if self.DEBUG < 1: return
        self.DEBUG_COUNT += 1
        print "%10d> %s" % (self.DEBUG_COUNT, message)

    #-----------------------------------------------------------------------
    #	M E T H O D S
    #-----------------------------------------------------------------------

    #	S L O T S    (that is, callbacks)

    def openFile(self):
        dialog = Dialog(gClient.GetRoot(), self.main)
        filename = dialog.SelectFile(kFDOpen, self.openDir)
        self.openDir = dialog.IniDir()
        self.statusBar.SetText(filename, 1)

        if filename[-5:] != '.root':
            dialog.ShowText("Oops!",
                    "Please select a root file",
                    230, 30)
            return
        self.__openFile(filename)

    def __openFile(self, filename):
        self.closeFile()		
        self.reader = TBFileReader(filename)
        self.nevents= self.reader.entries()
        self.statusBar.SetText('events: %d' % self.nevents, 0)
        self.eventNumber = -1
        self.nextEvent()
        self.wcplanes.CacheWCMeans("meanfile.txt", filename)

    def closeFile(self):
        try:
            if self.reader.file().IsOpen():
                self.reader.file().Close()
                del self.reader
        except:
            pass

    def setPage(self, id):
        self.noteBook.SetPage(id)
        if self.eventNumber >= 0:
            self.displayEvent()

    def nextEvent(self):
        self.debug("begin:nextEvent")
        self.readEvent(R_FORWARD)
        self.displayEvent()
        self.debug("end:nextEvent")			

    def previousEvent(self):
        self.debug('begin:previousEvent')
        self.readEvent(R_REWIND)
        self.displayEvent()
        self.debug('end:previousEvent')

    def gotoEvent(self):
        self.debug('begin:gotoEvent')
        from string import atoi
        dialog = Dialog(gClient.GetRoot(), self.main)
        self.eventNumber = atoi(dialog.GetInput('Goto event %d - %d' % \
                            (0, self.nevents-1),
                            '0'))
        self.eventNumber = max(self.eventNumber, 0)
        self.eventNumber = min(self.eventNumber, self.nevents-1)

        # do a one-shot read
        self.readEvent(R_ONESHOT)
        self.displayEvent()
        self.debug('end:gotoEvent')

    def forwardPlayer(self):
        self.debug('begin:forwardPlayer')
        self.mutex.Lock()
        self.forward = True
        self.timer.Start(self.DELAY, kFALSE)
        self.mutex.UnLock()
        self.debug('end:forwardPlayer')
        
    def rewindPlayer(self):
        self.debug('begin:rewindPlayer')
        self.mutex.Lock()
        self.forward = False
        self.timer.Start(self.DELAY, kFALSE)
        self.mutex.UnLock()
        self.debug('end:rewindPlayer')

    def stopPlayer(self):
        self.debug('begin:stopPlayer')
        self.mutex.Lock()
        self.timer.Stop()
        self.mutex.UnLock()
        self.debug('end:stopPlayer - STOP REQUESTED')

    def managePlayer(self):
        if self.forward:
            self.nextEvent()
        else:
            self.previousEvent()

    def toggleAccumulate(self):
        self.debug("toggle:accumulate")
        if self.accumulate==True:
            self.accumulate = False
            print "turning accumulate off"
        elif self.accumulate==False:
            self.accumulate==True
            print "turning accumulate on"
        else:
            self.accumulate = False
   	    print 'turned it off again' 
    def setDelay(self):
        from string import atof
        dialog = Dialog(gClient.GetRoot(), self.main)
        seconds= atof(dialog.GetInput('Enter delay in seconds', '2.0'))
        self.playerDelay = max(MINDELAY, int(1000*seconds))
        self.statusBar.SetText('delay set to: %8.1f s' % seconds, 1)

    def setADCcut(self):
        from string import atoi
        dialog = Dialog(gClient.GetRoot(), self.main)
        self.ADCcut = atoi(dialog.GetInput('Enter ADC cut', '500'))
        self.statusBar.SetText('ADC cut: %d' % self.ADCcut, 1)

    def close(self):
        gApplication.Terminate()

    def usage(self):
        dialog = Dialog(gClient.GetRoot(), self.main)
        dialog.SetText('Not done', 'Sorry!', 230, 30)
        #dialog.SetText('Help', HELP)

    def about(self):
        dialog = Dialog(gClient.GetRoot(), self.main)
        dialog.SetText('Not done', 'Sorry!', 230, 30)
        #dialog.SetText('About', ABOUT)

    def exit(self):
        self.close()

    def notdone(self):
        dialog = Dialog(gClient.GetRoot(), self.main)
        dialog.SetText('Not done', 'Sorry!', 230, 30)

    def run(self):
        gApplication.Run()


    #-----------------------------------------------------------------------
    # O T H E R   M E T H O D S
    #-----------------------------------------------------------------------

    # read events until we find one with a channel above specified ADC cut

    def readEvent(self, which=R_ONESHOT):
        self.debug("begin:readEvent")

        try:
            reader = self.reader
        except:
            dialog = Dialog(gClient.GetRoot(), self.main)
            dialog.SetText('Oops!', 'First open a root file',
                           230, 24)
            self.debug("end:readEvent")
            return


        # cache previous event number
        self.eventNumberPrev = self.eventNumber

        # loop over events and apply ADC cut

        if   which == R_ONESHOT:
            self.statusBar.SetText('event: %d' % self.eventNumber, 0)
            self.reader.read(self.eventNumber)

            ADCmax = self.reader.maxPadeADC()
            self.statusBar.SetText('max(ADC): %d' % ADCmax, 1)

        elif which == R_FORWARD:
            while self.eventNumber < self.nevents-1:
                self.eventNumber += 1
                self.statusBar.SetText('event: %d' % self.eventNumber, 0)
                self.reader.read(self.eventNumber)

                ADCmax =  self.reader.maxPadeADC()
                self.statusBar.SetText('max(ADC): %d' % ADCmax, 1)
                if ADCmax < self.ADCcut: continue
                break
        else:
            while self.eventNumber > 0:
                self.eventNumber -= 1
                self.statusBar.SetText('event: %d' % self.eventNumber, 0)
                self.reader.read(self.eventNumber)

                ADCmax =  self.reader.maxPadeADC()
                self.statusBar.SetText('max(ADC): %d' % ADCmax, 1)
                if ADCmax < self.ADCcut: continue				
                break			


        if self.eventNumber <= 0 or self.eventNumber >= self.nevents-1:
            self.stopPlayer()

        # Force a re-drawing of pages of notebook when a page is made
        # visible
        keys = self.noteBook.pages.keys()
        for key in keys:
            self.noteBook.pages[key].redraw = True

        self.debug("end:readEvent")
    #-----------------------------------------------------------------------
    def displayEvent(self):
        currentPage = self.noteBook.currentPage
        page = self.noteBook.pages[currentPage]
        self.debug("begin:displayEvent - %s" % page.name)
        if not page.redraw:
            self.debug("end:displayEvent - DO NOTHING")		
            return
        print 'self.reader.event() '
        print self.reader.event()
        self.display[page.name].Draw(self.reader.event())
        page.redraw = False
        self.debug("end:displayEvent")		
