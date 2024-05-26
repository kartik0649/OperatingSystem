#include <string>
#include <iostream>
#include <stdio.h>
#include <fstream> 
#include <unistd.h>
#include <cstring>
#include <vector>
#include <iterator>
#include <queue>
#include <deque>
#include <vector>
#include <set>
#include <bitset>
using namespace std;

string inputFile;
string randFile;
fstream input;
fstream rfile;
string line;
int num=0;
bool OFlag=false, PFlag=false, SFlag=false, FFlag=false;
int processCount=0;
const int VIRTUAL_PAGES = 64;
int MAX_FRAMES = 128;
char algo;
int handF;
int handC;
int handA;
int handTemp;
int handN;
int instCount=0;
int randSize;
int offset=0;
unsigned long contextSwitchCount=0 ,processExits=0;
unsigned long long totalCost=0;
vector<int>randomValues;

//void readRandomValues();


void readRandomValues()
{
    string num = "\0";
    rfile.open(randFile);
   // cout<<endl<<"GOT HERE0"<<endl;
    if(rfile.is_open()) {
       // cout<<endl<<"GOT HERE"<<endl;
        getline(rfile, num);
        randSize = stoi(num);
        //randvals = new int(stoi(num));
        for(int i = 0; getline(rfile, num); i++) {
                randomValues.push_back(stoi(num));
               // cout<<"READ"<<endl;
        }
    }
    else
    {
        cout<<"FILE DOES NOT EXIST"<<endl;
    }
    rfile.close();
}

int getRandomVal()
{   
    int ans= randomValues[offset]%MAX_FRAMES;
    offset++;
    offset=offset%randSize;
   // cout<<ans<<endl;
    return ans;
}


struct PTE
{
    unsigned int present : 1; 
    unsigned int write_protect : 1;
    unsigned int modified : 1;
    unsigned int referenced : 1;
    unsigned int pagedout : 1;
    unsigned int frame_number : 7;
    unsigned int filemap : 1;
    unsigned int exists : 1;

    void inittializePTE()
    {
        present = 0;
        write_protect = 0;
        modified = 0;
        referenced = 0;
        pagedout = 0;
        frame_number = 0;
        filemap = 0;
        exists = 0;
    }
};

struct FTE
{
    int processID;
    int vpageNum;
    int frameNum;
    int isVictim;
    unsigned long lastUsed;
    unsigned long age;
};
vector<FTE> frameTable;
deque<int> freeFrameQueue;



struct instructions
{
    char instType;
    int instTarget;
};

queue<instructions> instQueue;

struct VMA {
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
    // friend std::ostream& operator<<(std::ostream& os, const VMA& vma) {
    //     os << "VMA: Start=" << vma.start_vpage << " End=" << vma.end_vpage
    //        << " WriteProtected=" << (vma.write_protected ? "Yes" : "No")
    //        << " FileMapped=" << (vma.file_mapped ? "Yes" : "No");
    //     return os;
    // }
};

class Process {
public:
    vector<VMA> vmas;
    PTE page_table[VIRTUAL_PAGES];
    unsigned long long U, M, I, O, FI, FO, SV, SP, Z; 

    void setPageTable()
    {
       for(int i=0; i<VIRTUAL_PAGES; i++)
       {
         PTE e;
         e.inittializePTE();
         e.frame_number =0;
         page_table[i]=e;
       }
     U=M=I=O=FI=FO=SV=SP=Z=0;
    }

    //   void print() const {
    //     for (const auto& vma : vmas) {
    //         cout<<vma<<endl;
    //     }
    // }    
};
vector<Process> Processes;

class BasePager
{
public:
    virtual FTE *getVictimFrame() = 0;
};

class FIFO: public BasePager
{
 public:   
    FIFO()
    {
        handF=0;
    }
    FTE *getVictimFrame()
    {
        if(handF>=frameTable.size())
        {
            handF=0;
        }
        FTE *f=&frameTable[handF];
        f->isVictim=1;
        handF++;
        return f;
    }
};

class Random: public BasePager
{  
  public:
    Random(){}  

    FTE *getVictimFrame()
    {
      //  cout<<"HI"<<endl;
        int candIndex=getRandomVal();
     //   cout<<candIndex;
        FTE *cand=&frameTable[candIndex];
        cand->isVictim=1;
        return cand;
    }
};

class Clock: public BasePager
{
  public:
    Clock(){
        handC=0;
    }  

    FTE *getVictimFrame()
    {
        FTE *curr=&frameTable[handC];

        while(Processes[curr->processID].page_table[curr->vpageNum].referenced==1)
        {   
            Processes[curr->processID].page_table[curr->vpageNum].referenced=0;
            handC++;
            handC=handC%MAX_FRAMES;
            curr=&frameTable[handC];
        }
        if(Processes[curr->processID].page_table[curr->vpageNum].referenced==0)
        {
            handC++;
            handC=handC%MAX_FRAMES;
            curr->isVictim=1;
        }
         return curr;

    }
};

class NRU: public BasePager
{
  public:
    int lastUpdated; 
    int class0;
    int class1;
    int class2;
    int class3;
    NRU() 
    {
        lastUpdated=0;
        handN=0;
        int class0=-1;
        int class1=-1;
        int class2=-1;
        int class3=-1;

    }
    FTE *getVictimFrame()
    {
        //cout<<"Got here"<<endl;
        int classNo;
        FTE *cand=nullptr;
        int class0=class1=class2=class3=-1;
        for(int i=0;i<MAX_FRAMES;i++)
        {
            FTE * curr=&frameTable[(handN+i)%MAX_FRAMES];
            PTE * currPTE=&Processes[curr->processID].page_table[curr->vpageNum];
            if(currPTE->referenced==0 && currPTE->modified==0 && class0==-1)
            {
                classNo=0;
                class0=curr->frameNum;
                cand=curr;
                cand->isVictim=1;
                //  if(instCount < 50) 
                //         break;

            }
            else if(currPTE->referenced==0 && currPTE->modified==1 && class1==-1)
            {
                classNo=1;
                class1=curr->frameNum;
            }
            else if(currPTE->referenced==1 && currPTE->modified==0 && class2==-1)
            {
                classNo=2;
                class2=curr->frameNum;
            }
            else if(currPTE->referenced==1 && currPTE->modified==1 && class3==-1)
            {
                classNo=3;
                class3=curr->frameNum;
            }
        }

        if(instCount-lastUpdated>=48)
        {
            for(int i=0;i<MAX_FRAMES;++i)
            {
                FTE *j=&frameTable[i];
                if(j->processID!=-1 && Processes[j->processID].page_table[j->vpageNum].referenced==1)
                {
                    Processes[j->processID].page_table[j->vpageNum].referenced=0;
                }
            }
           lastUpdated=instCount; 
        }

        if(class0==-1)
        {
            if(class1 !=-1)
            {
                cand=&frameTable[class1];
            }

            else if(class2 !=-1)
            {
                cand=&frameTable[class2];
            }

            else if(class3 !=-1)
            {
                cand=&frameTable[class3];
            }
        }
        class0=-1;
        class1=-1;
        class2=-1;
        class3=-1;
        handN=(cand->frameNum +1)%MAX_FRAMES;
        cand->isVictim=1;
        return cand;
    }
};

class Aging: public BasePager
{
 public:
    Aging()
    {
        handA=0;
        handTemp=handA;
    }

    void modifyAge()
    {
        for(int i=0;i<MAX_FRAMES;i++)
        {
            FTE *f=&frameTable[(handTemp+i)%MAX_FRAMES];
            f->age=f->age >>1;
            if(Processes[f->processID].page_table[f->vpageNum].referenced==1)
            {
                 f->age = (f->age | 0x80000000);
                 Processes[f->processID].page_table[f->vpageNum].referenced=0;
            }
        }
    }

    FTE *getVictimFrame()
    {
        FTE *cand=&frameTable[(handA)%MAX_FRAMES];
        handTemp=handA;
        modifyAge();
        for(int i=0;i<MAX_FRAMES;i++)
        {
            FTE *f=&frameTable[(handA+i)%MAX_FRAMES];
             if(f->age < cand->age)
            {
                cand=f;
            }
        }
       // for(int i=0;i<MAX_FRAMES;i++)
       // {
       //      FTE *p =&frameTable[(handA+i)%MAX_FRAMES];
            // if(cand->age<p->age)
            // {
            //     cand=p;
            // }
      //  }
        handA=(cand->frameNum +1)%MAX_FRAMES;
        cand->isVictim=1;
        return cand;
    }
};

class WorkingSet: public BasePager
{
 public:
    FTE *currentFTE;
    PTE *currentPTE;
    FTE *ans;
    int handPos;
    int min;

      WorkingSet(){
        handPos = 0;
        min = -999;
        }

    FTE *getVictimFrame() {
        
        min = -999;
        ans = NULL;
        for(int i=0;i<MAX_FRAMES;i++)
        {
            currentFTE = &frameTable[(i+handPos)%MAX_FRAMES];
            currentPTE = &Processes[currentFTE->processID].page_table[currentFTE->vpageNum];
            int age = instCount - 1 - currentFTE->lastUsed;
           if (currentPTE->referenced==1)
            {
                currentPTE->referenced = 0;
                currentFTE->lastUsed = instCount - 1; 
            }
            else
            {
                if (age >= 50)
                {
                    ans = currentFTE;
                    ans->isVictim = 1;
                    handPos = (ans->frameNum + 1) % MAX_FRAMES;
                    return ans;
                }
                else
                {
                    if (age > min)
                    {
                        min = age;
                        ans = currentFTE;
                    }
                }
            }
        }
         if(ans == NULL)
          {
            ans = &frameTable[handPos];
          }
         handPos = (ans->frameNum + 1) % MAX_FRAMES;
         ans->isVictim = 1;
         return ans;
        
    }
};

BasePager *pager;


void setFrameTable()
{
    for(int i=0;i<MAX_FRAMES;i++)
    {
        FTE f;
        f.frameNum=i;
        f.processID=-1;
        f.vpageNum=-1;
        f.isVictim=0;
        f.lastUsed=0;
        f.age=0;
        frameTable.push_back(f);
    }

    for(int i=0;i<MAX_FRAMES;i++)
    {
        freeFrameQueue.push_back(i);
    }
}

FTE *getNewFrame()
{
     FTE *f;
    // cout<<"FreeFrame size is "<<freeFrameQueue.size()<<endl;
    if(freeFrameQueue.size()!=0)
    {
        int k=freeFrameQueue.front();
        freeFrameQueue.pop_front();
        for(int j=0;j<MAX_FRAMES;j++)
        {
            if(frameTable[j].frameNum==k)
            {
                f=&frameTable[j];
                return f;
            }
        }
    }
    else
    {
       // cout<<endl<<algo<<"1";
        f=pager->getVictimFrame();
       // cout<<endl<<algo<<"2";
    }
    return f;    

}




void parseInputs(int argc, char** argv) { 
    int c;  
    while((c = getopt(argc, argv, "f:a:o:")) != -1) {
        switch(c) {
             case 'f':{
                    int k=atoi(optarg);
                    MAX_FRAMES=k;
                   // cout<<"frame SIze is "<<MAX_FRAMES<<endl;
                    break;
             } 
             case 'a':{
                    char z=optarg[0];
                    if(z=='f')
                    {
                        algo='f';
                        break;
                    }
                    else if(z=='r')
                    {
                        algo='r';
                        break;
                    }
                    else if(z=='c')
                    {
                        algo='c';
                        break;
                    }
                    else if(z=='e')
                    {
                        algo='e';
                        break;
                    }
                    else if(z=='a')
                    {
                        algo='a';
                        break;
                    }
                    else if(z=='w')
                    {
                        algo='w';
                        break;
                    }
             }
                    
            
             case 'o':{
                   string options=optarg;
                    if (options.find('O') != string::npos) {
                        OFlag = true;
                    }
                    if(options.find('P') != string::npos) {
                        PFlag = true; 
                    }
                    if(options.find('F') != string::npos) {
                        FFlag = true;
                    }
                    if(options.find('S') != string::npos) {
                        SFlag = true; 
                    }
                   break;
             }
        }
    }

    // if(algo=='r')
    // {
    //     readRandomValues();
    // }
    
   // sscanf(stringOpt.c_str(), "%c%d:%d", &s, &quant, &maxprio);

}


void startSimulation()
{
    while(instQueue.size()!=0)
    {
        instCount++;
        instructions currInst=instQueue.front();
        instQueue.pop();
        cout<<instCount-1<<": ==> "<<currInst.instType<<" "<<currInst.instTarget<<endl;
        Process *currProc;
        int currProcId;
       // cout<<currInst.instType<<endl;
       // cout<<currInst.instTarget<<endl;
        if(currInst.instType=='c')
        {
           // cout<<"INC"<<endl;
            currProc=&Processes[currInst.instTarget]; 
            currProcId= currInst.instTarget;
            contextSwitchCount++;
            totalCost=totalCost+130;
            continue;
        }

        if(currInst.instType=='e')
        { 
          //  cout<<"INE"<<endl;
            cout<<"EXIT current process "<<currProcId<<endl;
            for(int i=0;i<VIRTUAL_PAGES;i++)
            {
                if(currProc->page_table[i].present==1)
                {
                    int occupiedFrameNum=currProc->page_table[i].frame_number;
                    FTE *occupedFrame= &frameTable[occupiedFrameNum];
                    cout<<" UNMAP "<<occupedFrame->processID<<":"<<occupedFrame->vpageNum<<endl;
                    occupedFrame->processID=-1;
                    occupedFrame->vpageNum=-1;
                    occupedFrame->isVictim=0;
                    occupedFrame->age=0;
                    occupedFrame->lastUsed=0;
                    totalCost=totalCost+ 410;
                    freeFrameQueue.push_back(occupiedFrameNum);
                    Processes[currInst.instTarget].U++;
                    currProc->page_table[i].frame_number=0;
                    if(currProc->page_table[i].modified==1)
                    {
                        currProc->page_table[i].frame_number =0;
                        if(currProc->page_table[i].filemap==1)
                        {
                            totalCost=totalCost + 2800;
                            cout<<" FOUT"<<"\n";
                            currProc->FO++;
                        }
                    }
                }
                    currProc->page_table[i].referenced=0;
                    currProc->page_table[i].modified=0;
                    currProc->page_table[i].present=0;
                    currProc->page_table[i].pagedout=0;
            }
            processExits++;
            totalCost=totalCost + 1230;
            continue;
        }
        
        PTE *pte=&currProc->page_table[currInst.instTarget];
        totalCost++;
        if(!pte->present)
        {
            bool isValid=false;
            if(pte->exists==1)
            {
                isValid=true;
            }
            else
            {
            for(int i=0;i<currProc->vmas.size();i++)
            {
               // cout<<"entered"<<endl;
                if(currInst.instTarget>=currProc->vmas[i].start_vpage && currInst.instTarget<=currProc->vmas[i].end_vpage)
                {
                    isValid=true;
                    pte->filemap=currProc->vmas[i].file_mapped;
                    pte->write_protect=currProc->vmas[i].write_protected;
                    pte->exists=1;
                    break;
                }
            }
            }
            if(isValid==false)
            {
                currProc->SV++;
                cout << " SEGV" << endl;
                totalCost=totalCost+440;
                continue;
            } 
            FTE *frame=getNewFrame();

            if(frame->isVictim==1)
            {
                //cout<<endl<<"FRAME NUMBER IS "<<frame->frameNum<<endl;
                int oldID=frame->processID;
                int oldPage=frame->vpageNum;
                totalCost=totalCost+410;

                 if(OFlag)
                {
                    cout<<" UNMAP "<<oldID<<":"<<oldPage<<endl;
                }

                //Processes[oldID].page_table->frame_number=-1;
                if(Processes[oldID].page_table[oldPage].modified==1)
                {
                    if(Processes[oldID].page_table[oldPage].filemap==1)
                    {
                        Processes[oldID].FO++;
                        cout<<" FOUT"<<endl;
                        totalCost=totalCost+2800;
                        Processes[oldID].page_table[oldPage].modified=0;
                    }
                    else
                    {
                        Processes[oldID].page_table[oldPage].modified=0;
                        Processes[oldID].page_table[oldPage].pagedout=1;
                        Processes[oldID].O++;
                        cout<<" OUT"<<endl;
                        totalCost=totalCost+2750;
                    }
                }
                Processes[oldID].U++;
                Processes[oldID].page_table[oldPage].present=0;
                Processes[oldID].page_table[oldPage].frame_number=0;
                frame->processID=-1;
                frame->vpageNum=-1;
            }

              pte->present=1;
          //  cout<<"PTE :";
          //  cout<<pte->present<<" "<<pte->modified<<" "<<pte->filemap<<" "<<pte->pagedout;
            if(pte->pagedout==1)
            {
               // cout<<"PAGEDOUT IS "<<pte->pagedout<<endl;
                if(pte->filemap==1)
                {
                    cout<<" FIN"<<endl;
                    totalCost=totalCost+2350;
                    currProc->FI++;
                }
                else
                {
                    cout<<" IN"<<endl;
                    totalCost=totalCost + 3200;
                    currProc->I++;
                }
            }
            else 
            {
               //  cout<<"PAGEDOUT IS "<<pte->pagedout<<endl;
                if(pte->filemap==1)
                {
                    cout<<" FIN"<<endl;
                    totalCost=totalCost + 2350;
                    currProc->FI++;
                }
                else
                {
                    cout<<" ZERO"<<endl;
                    totalCost=totalCost + 150;
                    currProc->Z++;
                }
            }
          //  cost=cost+350;
            if(OFlag)
            {
                cout<<" MAP "<<frame->frameNum<<endl;
            }
            frame->processID=currProcId;
            frame->vpageNum=currInst.instTarget;
            pte->frame_number=frame->frameNum;
            frame->lastUsed=instCount-1;
            currProc->M++;
            totalCost=totalCost+350;
        }


        if(currInst.instType=='r')
        {
            pte->referenced=1;
        }

        if(currInst.instType=='w')
        {
             pte->referenced=1;
             if(pte->write_protect==1)
             {
                cout<<" SEGPROT"<<endl;
                totalCost=totalCost + 410;
                currProc->SP++;
             }
             else
             {
                pte->modified=1;
             }
        }
    }
}


int main(int argc ,char** argv){

    parseInputs(argc, argv);
    inputFile = argv[optind];
    randFile= argv[optind+1];
    input.open(inputFile);
     if(algo=='r')
    {
        readRandomValues();
    }

    if(input.is_open()){
       while (getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        processCount =stoi(line);
        break; 
     }
     Processes.resize(processCount);

     while (getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        else break;
      }
     for(int i=0;i<processCount;i++)
      {
       //  cout<<"process is "<<i<<endl;
        int num_vmas= stoi(line);
      //  cout<<num_vmas<<endl;
        Processes[i].vmas.resize(num_vmas);
        for(int j=0;j<num_vmas;j++)
        {
            while(getline(input,line))
            {
                if (line.empty() || line[0] == '#') continue;
                int start_vpage, end_vpage, write_protected, file_mapped;
                sscanf(line.c_str(), "%d %d %d %d", &start_vpage, &end_vpage, &write_protected, &file_mapped);
                Processes[i].vmas[j]={start_vpage, end_vpage, static_cast<bool>(write_protected), static_cast<bool>(file_mapped)};
                break;
            }
            Processes[i].setPageTable();
        }
        if(i!=processCount-1)
        {
         while (getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        else break;
         }
        }

       }

        // while (getline(input, line)) {
        // if (line.empty() || line[0] == '#') continue;
        // else break;
        // }

        while(getline(input, line)){
            if (line.empty() || line[0] == '#') continue;
            char instType; 
            int  instTarget;
             sscanf(line.c_str(), "%c %d", &instType, &instTarget);
             instructions i={instType,instTarget};
             instQueue.push(i);
        }

        setFrameTable();

        if(algo=='f')
        {
            pager= new FIFO();
        }
        if(algo=='r')
        {
            pager= new Random();
        }
        if(algo=='c')
        {
            pager= new Clock();

        }
        if(algo=='e')
        {
            pager= new NRU();
        }
        if(algo=='a')
        {
            pager= new Aging();

        } 
        if(algo=='w')
        {
            pager=new WorkingSet();
        }   
               


        startSimulation();
    }
       // cout<<OFlag<<" "<<PFlag<<" "<<SFlag<<" "<<FFlag;
       if(PFlag)
       {
       // cout<<Processes.size()<<endl;
         for(int i= 0; i<Processes.size();i++) {
          // cout<<"nowHere"<<endl;
           Process curren=Processes[i];
           cout<<"PT["<<i<<"]:";
           for(int j=0;j<VIRTUAL_PAGES;j++)
           {
            cout<<" ";
            PTE page=curren.page_table[j];
            if(page.present)
            {
                cout<<j<<":";
                if(page.referenced == 1) {
                cout << "R"; 
                }
                else {
                cout << "-"; 
                }
                if(page.modified == 1) {
                cout << "M";
                }
                else {
                cout << "-"; 
                }
                if(page.pagedout == 1) {
                cout << "S"; 
                }
                else {
                cout << "-";
                }
            }

            else
            {
                if(page.pagedout==1)
                {
                    cout<<"#";
                }
                else
                {
                    cout<<"*";
                }
            }
           }
            cout<<endl;
           }
          
        }

        if(FFlag)
        {
            cout << "FT:"; 
            for(int i=0;i<MAX_FRAMES;i++)
            {
                cout<<" ";
                if(frameTable[i].processID!=-1)
                {
                     cout << frameTable[i].processID << ":" << frameTable[i].vpageNum;
                }
                else
                {
                     cout << "*";
                }
            }
        }

        if(SFlag)
        {
           // cout<<endl;
           // cost=(instCount -contextSwitchCount -processExits)*1  + contextSwitchCount*130 + processExits*1250; 
            for(int i=0;i<Processes.size();i++)
            {
                cout<<endl;
                printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu",i,
                Processes[i].U, Processes[i].M, Processes[i].I, Processes[i].O, Processes[i].FI, Processes[i].FO, Processes[i].Z, Processes[i].SV, Processes[i].SP);
            }
            cout<<endl;
             printf("TOTALCOST %lu %lu %lu %llu %lu\n", instCount, contextSwitchCount, processExits, totalCost, sizeof(PTE)); 
             //cout<<endl;
        }

    
    
    //  for (size_t i = 0; i < Processes.size(); ++i) {
    //     std::cout << "Process " << i << ":" << std::endl;
    //     Processes[i].print();
    //     std::cout << std::endl;
    // }

}



