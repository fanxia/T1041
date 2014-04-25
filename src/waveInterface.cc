// ROOT Interface for viewing waveform's generated from the test shashlik module
// Initial Version 0.00000001 
// Thomas Anderson

#include <iostream> 
#include <vector>
using std::vector; 


//Root Includes
#include <TSystem.h>
#include <TObject.h>
#include <TInterpreter.h>
#include <TApplication.h> 
#include "TString.h"
#include "TSpectrum.h"
#include "TList.h"
#include "TPolyMarker.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TFitResultPtr.h"
#include "TF1.h"
#include <TNtuple.h>
#include "TTree.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TQObject.h"
#include <TGButton.h>
#include <TGClient.h>
#include <TGLabel.h>
#include "TGFileBrowser.h"
#include "TROOT.h"
#include "TMath.h"
#include "TRegexp.h"
#include "TArrayF.h"
#include "TArrayC.h"
#include <TGSlider.h>


#include "waveInterface.h"


// Button Frame Constants
const UInt_t mainFrameWidth = 1000; 
const UInt_t mainFrameHeight = 750; 
const UInt_t buttonFrameWidth = 500; 
const UInt_t buttonFrameHeight = 250; 
const UInt_t spectrumFrameWidth = 1000; 
const UInt_t spectrumFrameHeight = 500; 


BoardMap::BoardMap() { 
  board = 0; 
}

BoardMap::BoardMap(UInt_t bid) { 
  board = bid; 
}


void *timer(void *ptr) { 
  waveInterface *interface = (waveInterface *) ptr; 

  while(1) { 
    if (!interface->PlayerStatus())
      break; 

    if (interface->nextChannel()) {
      interface->nextEntry(); 
      interface->firstChannel(); 
    }
    gSystem->Sleep(interface->Delay());  
  }
  return 0; 
}


waveInterface::waveInterface(bool initialise) { 
  _selectedChannels = 0; 
  _f = NULL; 
  _filename = "" ;
  _width = 1000; 
  _height = 500; 
  _playerStatus = false; 
  _delay=1000;
  _currentBoard = -1; 
  //By default start the interface
  if (initialise)
    initWindow(); 
}

waveInterface::waveInterface(const TString &filename, bool initialise) { 
  _selectedChannels = 0; 
  _f = NULL; 
  _filename = filename; 
  _playerStatus = false; 
  _currentBoard = -1; 

  std::cout << "Just starting up" << std::endl; 
  _width = 1000; 
  _height = 500; 
  if (initialise) {
    initWindow(); 
  }
  
}

waveInterface::~waveInterface() { 
  if (_FMain) {
    delete _FMain; 
  }

}



void waveInterface::initWindow(UInt_t width, UInt_t height) { 


  if (width != 0 && height != 0) {
    _width = width; 
    _height = height; 
  }


  _FMain = new TGMainFrame(gClient->GetRoot(), _width, _height, kVerticalFrame); 
  _FMain->SetWindowName("Test Beam Waveform Viewer v.00000001"); 
  _buttonFrame = new TGVerticalFrame(_FMain, buttonFrameWidth, buttonFrameHeight, kFixedWidth | kFixedHeight); 
  _spectrumFrame = new TGVerticalFrame(_FMain, spectrumFrameWidth, spectrumFrameHeight, kLHintsLeft | kFixedWidth); 
  _waveformCanvas = new TRootEmbeddedCanvas("waveform", _spectrumFrame, spectrumFrameWidth, spectrumFrameHeight, kSunkenFrame); 
  _spectrumFrame->AddFrame(_waveformCanvas, new TGLayoutHints(kLHintsExpandY, 10,10,10,1)); 

   makeButtons(); 
   connectButtons(); 
   if (_filename != "") { 
     loadRootFile(); 
   }
  _FMain->MapSubwindows(); 
  _FMain->Resize(_FMain->GetDefaultSize()); 
  _FMain->MapWindow(); 

}




void waveInterface::addCheckBoxes() { 
  std::cout << " Adding Board and channel buttons." << std::endl; 
  TGRadioButton *brd = NULL; 
  TGCheckButton *ch = NULL; 
  TString converter; 

  //Update the board/channel map for the current event
  _boards.clear(); 
  buildChannelBoardMap(_event, _boards);
  //  std::cout << _boards.size() << " Boards "<< std::endl; 

  if (_boardFrame != NULL) { 
    //    std::cout << "Clearing old board frame. " << std::endl; 
    _buttonFrame->RemoveFrame(_boardAndchannelFrame); 
    delete _boardAndchannelFrame; 
    _chList.Clear(); 
    _boardList.Clear(); 
    
  }
  _chList.Expand(32); 
  _boardList.Expand(4); 


  //Build board frame
  _boardAndchannelFrame = new TGHorizontalFrame(_buttonFrame); 

  _boardFrame = new TGGroupFrame(_boardAndchannelFrame, "Boards", kVerticalFrame); 
  for (UInt_t i = 0; i < _boards.size(); i++) { 
    BoardMap *m = _boards[i]; 
    //    std::cout << "Board ID:" << m->board << " # Chs: " << m->chs.size() << std::endl; 
    brd = new TGRadioButton(_boardFrame, TGHotString(converter.Itoa(m->board, 10))); 
    _boardList.Add(brd); 
  
    if (i == 0 && _currentBoard == -1) { 
      brd->SetState(kButtonDown); 
      _selected = brd; 
      _currentBoard = 0; 
    }
    else if (m->board == _boards[_currentBoard]->board) { 
      brd->SetState(kButtonDown); 
      _selected = brd; 
      _currentBoard = i; 
      std::cout << "Current Board: " << _boards[i]->board << std::endl; 
    }

    brd->Connect("Clicked()", "waveInterface", this, "updateBoardSelection()"); 
    _boardFrame->AddFrame(brd, new TGLayoutHints(kLHintsTop,5,5,5,1)); 

  }


  _channelFrame = new TGCompositeFrame(_boardAndchannelFrame); 




  _channelFrame->SetLayoutManager(new TGMatrixLayout(_channelFrame, 9, 0, 1)); 
  
  UInt_t row = 0; 
  UInt_t check = 8; 
  for (UInt_t i = 0; i < _boards[_currentBoard]->chs.size(); i++) { 
    ch = new TGCheckButton(_channelFrame, TGHotString(converter.Itoa(i, 10))); 
    ch->Connect("Clicked()", "waveInterface", this, "updateChannelSelection()"); 
    _channelFrame->AddFrame(ch); 
    _chList.Add(ch); 
    row++; 
    // if (row == check) { 
    //   ch = new TGCheckButton(_channelFrame, TGHotString("All")); 
    //   ch->Connect("Clicked()", "waveInterface", this, "selectChannelRow()"); 
    //   _channelFrame->AddFrame(ch); 
    //   _chList.Add(ch); 
    //   check += 8; 
    // }
  }

  for (UInt_t i = 0; i < _chList.GetEntries(); i++) {
    if (_selectedChannels.TestBitNumber(i)) {
      ((TGCheckButton *) _chList[i])->SetState(kButtonDown); 
      //      std::cout << "Selected Channel:" << i << " Pade Channel " << _boards[_currentBoard]->chs[i] << std::endl; 
    }
  }

  if (_selectedChannels.CountBits() == 0) { 
    _currentChannel = 0; 
  }
  _boardAndchannelFrame->AddFrame(_boardFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom | kFixedWidth, 5,5,5,1)); 
  _boardAndchannelFrame->AddFrame(_channelFrame, new TGLayoutHints(kLHintsRight ,5,5,5,1)); 
  _buttonFrame->AddFrame(_boardAndchannelFrame, new TGLayoutHints(kLHintsExpandX, 5,5,5,1)); 
  _buttonFrame->MapSubwindows(); 
  _buttonFrame->Resize(); 
  
}

void waveInterface::makeButtons()  {
  std::cout << "Adding buttons to display." << std::endl; 
  ULong_t red;
  ULong_t green;
  ULong_t blue;
  ULong_t gray;
  ULong_t white;
  gClient->GetColorByName("red", red);
  gClient->GetColorByName("green", green);
  gClient->GetColorByName("blue", blue);
  //  gClient->GetColorByName("gray", gray);
  gClient->GetColorByName("#888888",gray);
  gClient->GetColorByName("white", white);

  _actionFrame = new TGHorizontalFrame(_buttonFrame); 


  

  _loadBN = new TGTextButton(_actionFrame, "&Load File");
  _loadBN->SetTextColor(blue);
  _loadBN->Connect("Clicked()", "waveInterface", this, "openFileDialog()");
  _loadBN->SetToolTipText("Select input ROOT file", 2000);
  _actionFrame->AddFrame(_loadBN, new TGLayoutHints(kLHintsTop,5,5,5,1)); 


  _firstchBN = new TGTextButton(_actionFrame, "&First Channel");

  _actionFrame->AddFrame(_firstchBN, new TGLayoutHints(kLHintsTop,5,5,5,1));

  _prevchBN = new TGTextButton(_actionFrame, "&Prev Channel");

  _actionFrame->AddFrame(_prevchBN, new TGLayoutHints(kLHintsTop,5,5,5,1));

  _nextchBN = new TGTextButton(_actionFrame, "&Next Channel");

  _actionFrame->AddFrame(_nextchBN, new TGLayoutHints(kLHintsTop,5,5,5,1));


  _prevenBN = new TGTextButton(_actionFrame, "&Prev Entry");

  _actionFrame->AddFrame(_prevenBN, new TGLayoutHints(kLHintsTop,5,5,5,1));

  _nextenBN = new TGTextButton(_actionFrame, "&Next Entry");
 
  _actionFrame->AddFrame(_nextenBN, new TGLayoutHints(kLHintsTop,5,5,5,1));



  _goBN = new TGTextButton(_actionFrame, "&Go");
  _actionFrame->AddFrame(_goBN, new TGLayoutHints(kLHintsTop,5,5,5,1));

  _stopBN = new TGTextButton(_actionFrame, "&Stop");
  _actionFrame->AddFrame(_stopBN, new TGLayoutHints(kLHintsTop,5,5,5,1)); 


  _delayBox = new TGNumberEntry(_actionFrame, 1000,9,999, TGNumberFormat::kNESInteger,
				TGNumberFormat::kNEANonNegative,TGNumberFormat::kNELLimitMinMax,100,10000);
  _actionFrame->AddFrame(_delayBox, new TGLayoutHints(kLHintsTop,5,5,5,1)); 

  //  _slider = new TGHSlider(_actionFrame, 0, 200, kSlider1 | kScaleBoth, 100); 
  //  _slider->SetRange(0, 31); 
  //  _actionFrame->AddFrame(_slider); 


  _buttonFrame->AddFrame(_actionFrame, new TGLayoutHints(kLHintsExpandX | kLHintsTop,5,5,5,1)); 
  _FMain->AddFrame(_spectrumFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY)); 
  _FMain->AddFrame(_buttonFrame, new TGLayoutHints(kLHintsExpandX,10,10,10,1)); 



}


void waveInterface::connectButtons()  {
  
  std::cout << "Connecting buttons" << std::endl; 

  _goBN->Connect("Clicked()", "waveInterface", this, "Go()"); 
  _stopBN->Connect("Clicked()", "waveInterface", this, "Stop()"); 
  _nextenBN->Connect("Clicked()", "waveInterface", this, "nextEntry()"); 
  _prevenBN->Connect("Clicked()", "waveInterface", this, "prevEntry()");

  _nextchBN->Connect("Clicked()", "waveInterface", this, "nextChannel()"); 
  _prevchBN->Connect("Clicked()", "waveInterface", this, "prevChannel()"); 
  _firstchBN->Connect("Clicked()", "waveInterface", this, "firstChannel()"); 
  _delayBox->Connect("ReturnPressed()", "waveInterface", this, "delayBoxUpdate()"); 
  _delayBox->Connect("ValueSet(Long_t)", "waveInterface", this, "delayBoxUpdate()"); 


  _FMain->Connect("CloseWindow()", "waveInterface", this, "closeCleanup()"); 

}

void waveInterface::selectChannelRow() { 
  std::cout << "Currently not implemented!" << std::endl; 


}

void waveInterface::updateChannelSelection() { 
  //Update the selected channels, I'm using an unsigned long (8 bytes) to store the 
  // status of each channel

  // The lowest order bit indicates channel 0, 7 6 5 4 3 2 1 0
  // and goes up from there 
  ULong64_t proto = 0; 
  _selectedChannels.Set(64, &proto); 


  TGRadioButton *chosen; 
  for (UInt_t i = 0; i < _chList.GetEntries(); i++) { 
    chosen = (TGRadioButton *) _chList[i]; 
    if (chosen->IsOn()) { 
      //This also needs replacing, very brittle approach to getting the channel number
      Int_t channel = chosen->GetString().Atoi(); 
      _selectedChannels[channel] = 1; 
    }
  }



      
  

} 

void waveInterface::updateBoardSelection() {
  //Note: I find this fairly hacky, it works but should get replaced 
  _selected->SetState(kButtonUp); 
  TGRadioButton *chosen = NULL; 

  Bool_t found = false; 
  Int_t i =0; 
  for (i = 0; i < _boardList.GetEntries(); i++) { 
    chosen = (TGRadioButton *) _boardList[i]; 
    if (chosen->IsOn()) { 
      found = true; 
      std::cout << "You selected Board:" << chosen->GetString() << std::endl; 
      _selected = chosen; 
      _currentBoard = i; 
      break; 
    }
  }
  if (!found)  {
    //_selected must have been selected
    _selected->SetState(kButtonDown); 
  }
  else { 
    chosen->SetState(kButtonDown); 
    BoardMap *m = _boards[i]; 
    updateFrame(_currentEntry, m->chs[_currentChannel]); 

    UInt_t entries = _chList.GetEntries(); 
    std::cout << "Entries:" << entries << std::endl; 
    if (m->chs.size() > entries) { 
      std::cout << "Changing channel check boxes" << std::endl; 
      _currentBoard = i; 
      
    }
  }

}

void waveInterface::delayBoxUpdate() { 

  _delay = _delayBox->GetIntNumber(); 

}

void waveInterface::closeCleanup() { 

  std::cout << "Got window close or Alt-F4, quitting!" << std::endl; 
  exit(0); 

}

void waveInterface::reset() { 
  _waveformCanvas->GetCanvas()->Clear(); 
  delete _waveform; 
  _waveform = NULL; 
  _f->Close(); 
  _event = NULL;
  _eventTree = NULL; 
  _waveform = NULL; 

  

}

void waveInterface::openFileDialog() { 

  std::cout << "Open something will ya!" << std::endl; 
  TGFileInfo *fileInfo = new TGFileInfo; 
  TGFileDialog *fileDialog = new TGFileDialog(gClient->GetRoot(), _FMain, kFDOpen, fileInfo); 
  std::cout << "Selected file: " << fileInfo->fFilename << std::endl; 
  
  
  if (fileInfo->fFilename == NULL) {
    std::cout << "File selection cancelled." << std::endl; 
    return; 
  }

  _filename = fileInfo->fFilename; 
  loadRootFile(); 

}

void waveInterface::Go() { 

  std::cout << "Starting channel playback..." << std::endl; 
  _playerStatus = true; 
  _timer = new TThread("timer", timer, this); 
  _timer->Run(); 

}

void waveInterface::Stop() { 
  std::cout << "Stopping channel playback..." << std::endl; 
  _playerStatus = false; 

}

void waveInterface::updateFrame(UInt_t entry, UInt_t channel) { 
  //Quick sanity check
  if (_f == NULL or _eventTree == NULL)
    return;

  _eventTree->GetEntry(entry); 
  _padeChannel = _event->GetPadeChan(channel); 
  _padeChannel.GetHist(_waveform); 

  _waveformCanvas->GetCanvas()->cd(); 
  std::cout << "Drawing Sample" << std::endl; 
  _waveform->Draw(); 

  _waveformCanvas->GetCanvas()->Modified(); 
  _waveformCanvas->GetCanvas()->Update(); 
}


void waveInterface::buildChannelBoardMap(TBEvent *evt, vector<BoardMap *> &brds) {
  //  std::cout << "Building Channel Board Map" << std::endl; 
  BoardMap *map = NULL; 
  for (Int_t ch = 0; ch < evt->NPadeChan(); ch++) { 
    if (map == NULL) { 
      map = new BoardMap(); 
      map->board = evt->GetPadeChan(ch).GetBoardID(); 
    }
    else if (map->board != evt->GetPadeChan(ch).GetBoardID()) {
      brds.push_back(map); 
      map = new BoardMap();
      map->board = evt->GetPadeChan(ch).GetBoardID(); 
    }
	 
    map->chs.push_back(ch); 
  }
  

}
    

void waveInterface::loadRootFile() { 


  if (_f != NULL && _f->IsZombie()) { 
    std::cout << "Closing stale root file" << std::endl; 
    delete _f; 
    _f = NULL; 
  }

  if (_f) { 
    std::cout << "Cleaning old data" << std::endl; 
    reset(); 

  }


  if (_filename == "")
    return; 


  std::cout << "Loading Root File" << std::endl; 
  _f = new TFile(_filename);


  
  _currentBoard = -1; 
  

  std::cout << "Getting events from Tree" << std::endl; 
  _event = new TBEvent(); 
  _eventTree = (TTree*)_f->Get("t1041"); 
  _bevent = _eventTree->GetBranch("tbevent"); 
  _bevent->SetAddress(&_event); 
  
  std::cout << "Building waveform histogram" << std::endl; 
  _waveform = new TH1F("hw", "waveform", 120, 0, 120);
  

  std::cout << "Getting test sample" << std::endl; 
  _eventTree->GetEntry(0); 
    
  
  _currentEntry = 0; 
  _currentChannel = 0; 



  _padeChannel = _event->GetPadeChan(0); 


  _padeChannel.GetHist(_waveform); 


  _waveformCanvas->GetCanvas()->cd(); 
  std::cout << "Drawing Sample" << std::endl; 
  addCheckBoxes(); 

  _waveform->Draw(); 

  _waveformCanvas->GetCanvas()->Modified(); 
  _waveformCanvas->GetCanvas()->Update(); 


}


void waveInterface::firstChannel() { 

  if (_selectedChannels.CountBits() == 0) {
    std::cout << "No channels selected." << std::endl; 
    ((TGCheckButton *) _chList[0])->SetState(kButtonDown); 
    _currentChannel = _boards[_currentBoard]->chs[0]; 
  }
  else {
    UInt_t pos = _selectedChannels.FirstSetBit(); 
    _currentChannel = pos; 
    std::cout << "First Set Channel:" << pos << " In Pade: " << _boards[_currentBoard]->chs[_selectedChannels.FirstSetBit()] << std::endl; 
  }
  updateFrame(_currentEntry, _boards[_currentBoard]->chs[_currentChannel]); 
}

bool waveInterface::nextChannel() { 

  BoardMap *m = (BoardMap *) _boards[_currentBoard]; 
  Int_t maxchans =  m->chs.size(); 
  std::cout << _currentChannel  << " /" << m->chs.size() << " Channels." << std::endl; 
  for (Int_t i =_currentChannel+1; i < maxchans; i++) { 
    if (_selectedChannels.TestBitNumber(i))
      {
	_currentChannel = i; 
	std::cout << "Choosing channel: "  << i << std::endl; 
	std::cout << "This channel maps to: " << m->chs[i] << std::endl; 

	updateFrame(_currentEntry, m->chs[i]); 
	return false; 
      }
  }
  
  return true; 
}

void waveInterface::prevChannel() { 
  if (_currentChannel == 0) 
    return; 

  BoardMap *m = (BoardMap *) _boards[_currentBoard]; 

  for (Int_t i = (_currentChannel-1); i >= 0; i--) { 
    if (_selectedChannels.TestBitNumber(i))
      {
	_currentChannel = i; 
	std::cout << "Choosing channel: "  << i << std::endl; 
	std::cout << "This channel maps to: " << m->chs[i] << std::endl; 
	updateFrame(_currentEntry, m->chs[i]); 
	break; 
      }
  }
}



bool waveInterface::nextEntry() { 
  bool finished = true; 
  Int_t maxentries = _eventTree->GetEntries(); 
  std::cout << "Current Entry:" << _currentEntry << " of: " << maxentries << std::endl; 

  if (_currentEntry < maxentries) {
    _currentEntry++; 
    finished = false; 
  }

  updateFrame(_currentEntry, _boards[_currentBoard]->chs[_currentChannel]); 
  addCheckBoxes(); 

  return finished; 
}

void waveInterface::prevEntry() { 
  Int_t maxentries = _eventTree->GetEntries(); 
  std::cout << "Current Entry:" << _currentEntry << " of: " << maxentries << std::endl; 

  if (_currentEntry == 0)
    return; 
  _currentEntry--; 
  addCheckBoxes(); 
  updateFrame(_currentEntry, _boards[_currentBoard]->chs[_currentChannel]); 
  

}
