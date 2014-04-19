T1041
=====

Analysis code for T1041 test beam run

Make TTrees from TB data 
========================
Usage:  
python TBNtupleMaker [OPTION] PADE_FILE [WC_FILE]  
       Options  
       -n max_events  : Maximum number of events to read"  
Produces an output file with the same basename as the PADE_FILE, replacing .txt(.bz2) with .root

Display channel mapping  
======================  
root -l testChannelMap.C+


Simple event display  
====================  
# Make plots averaged over all events -- saves .gif files  
python dqmPlots.py filename.root  

# Make plots for a single event -- saves .gif files  
python dqmPlots.py filename.root eventNumber  

Simple wave fom scanner  
=======================  
python waveViewer.py rec_capture_20140413_003028.root



Make png plots and html page from a root file
=============================================
python root2html.py file.root  
  png files and an index.html file are produced in directory called "file", 
the root file name w/o the .root


Run Jordan's WC display  
=======================  
python WC_Reader.py file.root


git cheatsheet
==============

clone a working copy

HTTPS clone: git clone https://github.com/ShashlikTB/T1041.git

SSH clone (do this to allow password-less check in w/ ssh key):  
git clone git@github.com:ShashlikTB/T1041.git


update/add:
* git add <file>
* git commit -m "comment"

check it in:
* git push origin master


