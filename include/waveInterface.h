#ifndef WAVEVIEWER_H
#define WAVEVIEWER_H



#include "TGFrame.h"
#include "TRootEmbeddedCanvas.h"
#include "TGFileDialog.h"
#include "TGCanvas.h"
#include "TGStatusBar.h"
#include "TGNumberEntry.h"
#include "TArrow.h"
#include "TPad.h"
#include "TH1F.h"
#include "TPaveLabel.h"
#include "TPaveText.h"
#include "TFile.h"
#include "TString.h"
#include "TTree.h"
#include "RQ_OBJECT.h"
#include "TThread.h"
#include "TGSlider.h"
#include "TBEvent.h"
#include <vector>


using std::vector; 

struct BoardMap { 
  UInt_t board; 
  vector<UInt_t> chs; 

  BoardMap(); 
  BoardMap(UInt_t bid); 
}; 





class waveInterface : public TQObject { 

  RQ_OBJECT("waveInterface"); 
  ClassDef(waveInterface,1);

 private: 
  void loadRootFile(); 

  TString _filename; 
  TGMainFrame *_FMain; 
  TGVerticalFrame *_buttonFrame; 
  TGHorizontalFrame *_boardAndchannelFrame; 
  TGHorizontalFrame *_actionFrame; 
  TGVerticalFrame *_spectrumFrame; 
  TGCompositeFrame *_channelFrame; 
  TGGroupFrame *_boardFrame; 



  TRootEmbeddedCanvas   *_waveformCanvas;

  TBEvent *_event; 
  TTree *_eventTree; 
  TBranch *_bevent; 
  TH1F *_waveform; 
  TThread *_timer; 

  UInt_t _delay; 
  UInt_t _width; 
  UInt_t _height; 
  Int_t _currentEntry; 
  Int_t _currentChannel; 
  Int_t _currentBoard; 
  bool _playerStatus; 

  //Buttons 
  TGTextButton *_loadBN;
  TGTextButton *_goBN;
  TGTextButton *_stopBN;
  TGTextButton *_nextenBN;
  TGTextButton *_prevenBN;

  TGTextButton *_nextchBN;
  TGTextButton *_prevchBN;
  TGTextButton *_firstchBN;

  //Channel and Board Button List
  TObjArray _chList; 
  TObjArray _boardList; 

  TGRadioButton *_selected; 
  vector<BoardMap *> _boards;   
  //  TGHSlider *_slider; 
  TGNumberEntry *_delayBox; 

  


  TFile *_f; 
  
  PadeChannel _padeChannel; 
  


 public: 
  waveInterface(const TString &filename, bool initialise=true); 
  waveInterface(bool initialise=true); 
  virtual ~waveInterface(); 
  void openFileDialog(); 
  void closeCleanup(); 
  void Go(); 
  void Stop(); 

  bool PlayerStatus() { return _playerStatus; } 
  UInt_t Delay() { return _delay; } 
  
  bool nextChannel(); 
  void prevChannel(); 
  bool nextEntry(); 
  void prevEntry(); 
  void firstChannel(); 
  void updateBoardSelection(); 


  void setDelay(UInt_t delay) { _delay = delay;  }
  void delayBoxUpdate();
  void buildChannelBoardMap(TBEvent *evt, vector<BoardMap *> &brds); 

  void initWindow(UInt_t width=0, UInt_t height=0); 
  void makeButtons(); 
  void addCheckBoxes(); 
  void connectButtons(); 
  void reset(); 
  void updateFrame(UInt_t entry, UInt_t channel); 
  
}; 

#endif 
