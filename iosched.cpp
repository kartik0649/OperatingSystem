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
#include <algorithm> 
using namespace std;

string inputFile;
fstream input;
string line;
int totalTime=0;
int currTrackPos=0;
int totalTrackMovement=0;
int size;
double avg_turnaround=0;
double avg_waittime =0;
int max_waittime=0;

bool direction=true; // true means increasing , false means decreasing .

struct inst
{
    int timeStep;
    int track;
    int startTime;
    int endTime;
};

queue<inst>instructionQueue;
deque<inst>inputQ;
deque<inst>completedQ;
deque<inst>activeQ;
deque<inst>addQ;
inst active={-1,-1,-1,-1};

bool compareByTimeStep(const inst& a, const inst& b) {
    return a.timeStep < b.timeStep;
}


class BaseIoScheduler
{
public:
    virtual inst strategyGetNext() = 0;
};

class FIFO : public BaseIoScheduler
{
public:
    virtual inst strategyGetNext(){
        inst f= inputQ.front();
        inputQ.pop_front();
        return f;
    }
};

class SSTF: public BaseIoScheduler
{
public:
    virtual inst strategyGetNext(){
        inst f;
        if(inputQ.size()==1)
        {
            f=inputQ.front();
            inputQ.pop_front();
            return f;
        }
        int MaxTrack=10000;
        auto minPos=inputQ.begin();
        for(auto i = inputQ.begin(); i!=inputQ.end();++i)
        {
            if(abs((*i).track-currTrackPos)<MaxTrack)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }
        }
        inputQ.erase(minPos);
        return f;
    }
};

class LOOK : public BaseIoScheduler
{
public:
    virtual inst strategyGetNext(){
        inst f={-1,-1,-1,-1};
        if(inputQ.size()==1)
        {
            f=inputQ.front();
            inputQ.pop_front();
            return f;
        }
         int MaxTrack=10000;
         auto minPos=inputQ.begin();
        for(auto i = inputQ.begin(); i!=inputQ.end();++i)
        {
            if(abs((*i).track-currTrackPos)<MaxTrack  && direction==true && (*i).track >=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }

             if(abs((*i).track-currTrackPos)<MaxTrack  && direction==false && (*i).track <=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }
        }
        if(f.startTime==-1)
        {
             for(auto i = inputQ.begin(); i!=inputQ.end();++i)
           {
            if(abs((*i).track-currTrackPos)<MaxTrack  && direction==false && (*i).track >=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }

             if(abs((*i).track-currTrackPos)<MaxTrack  && direction==true && (*i).track <=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }
           }
        }
        inputQ.erase(minPos);
        return f;

    }
};


class CLOOK : public BaseIoScheduler
{
public:
    virtual inst strategyGetNext(){
        inst lf={-1,-1,-1,-1};
        inst rn={-1,-1,-1,-1};
        inst ans;
        bool lset=false;
        bool rset=false;
        auto minPos1=inputQ.begin();
        auto minPos2=inputQ.begin();

        int lfar = 10000;
        int rnear = 10000;

        for(auto i = inputQ.begin(); i!=inputQ.end();++i)
        {
            int target=(*i).track;

            if(target>=currTrackPos)
            {
                if(target<rnear)
                {
                    rnear=target;
                    rn={(*i).timeStep, (*i).track, (*i).startTime, (*i).endTime};
                    minPos1=i;
                    rset=true;
                }
            }
            else
            {
                if(target<lfar)
                {
                    lfar=target;
                    lf={(*i).timeStep, (*i).track, (*i).startTime, (*i).endTime};
                    minPos2=i;
                    lset=true;
                }
            }
        }

        if(direction==true)
        {
            if(rset)
            {
                ans=rn;
                inputQ.erase(minPos1);
            }
            else
            {
                ans=lf;
                inputQ.erase(minPos2);
                direction=false;
            }
        }

        else
        {
            if(rset)
            {
                ans=rn;
                inputQ.erase(minPos1);
                direction=true;
            }
            else
            {
                inputQ.erase(minPos2);
                ans=lf;
            }
        }

        return ans;

    }
};

class FLOOK : public BaseIoScheduler
{
public:
    virtual inst strategyGetNext(){
        inst f={-1,-1,-1,-1};
            if(activeQ.empty()) {
                if(!inputQ.empty())
                {
                    inputQ.swap(activeQ);
                }
                else 
                    return f;
            }

        if(activeQ.size()==1)
        {
            f=activeQ.front();
            activeQ.pop_front();
            return f;
        }
         int MaxTrack=10000;
         auto minPos=activeQ.begin();
        for(auto i = activeQ.begin(); i!=activeQ.end();++i)
        {
            if(abs((*i).track-currTrackPos)<MaxTrack  && direction==true && (*i).track >=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }

             if(abs((*i).track-currTrackPos)<MaxTrack  && direction==false && (*i).track <=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }
        }
        if(f.timeStep==-1)
        {
             for(auto i = activeQ.begin(); i!=activeQ.end();++i)
           {
            if(abs((*i).track-currTrackPos)<MaxTrack  && direction==false && (*i).track >=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }

             if(abs((*i).track-currTrackPos)<MaxTrack  && direction==true && (*i).track <=currTrackPos)
            {
                MaxTrack=abs((*i).track-currTrackPos);
                f=(*i);
                minPos=i;
            }
           }
        }
        if(f.timeStep!=-1)
        {
        activeQ.erase(minPos);
        }
        return f;
            
    }
};

BaseIoScheduler *algo;


void completeIO()
{
    active.endTime=totalTime;
}

void startSimulation()
{
    while(true)
    {
      
        if(!instructionQueue.empty())
        {
            inst i=instructionQueue.front();
            if(totalTime==i.timeStep)
            {
                inputQ.push_back(i);
                instructionQueue.pop();
            }
        }

        
        if(active.timeStep!=-1 && active.track==currTrackPos)
        {
            completeIO();
            inst j ={active.timeStep,active.track,active.startTime,active.endTime};
            completedQ.push_back(j);
            //find a method to remove element from input queue after completion.
            active={-1,-1,-1,-1};
            if(size==completedQ.size())
            {
                break;
            }
            continue;
        }

        if(active.timeStep!=-1 && active.track!=currTrackPos)
        {
             if(active.track>currTrackPos)
                {
                    currTrackPos++;
                    direction=true;
                    totalTrackMovement++;
                }
                else
                {
                    currTrackPos--;
                    direction=false;
                    totalTrackMovement++;
                }
        }


        if(active.timeStep==-1 && (!inputQ.empty() || !activeQ.empty()))
        {
            active=algo->strategyGetNext();
            active.startTime=totalTime;
            if(active.track==currTrackPos)
            {
                completeIO();
                inst j ={active.timeStep,active.track,active.startTime,active.endTime};
                completedQ.push_back(j);
                active={-1,-1,-1,-1};
                if(size==completedQ.size())
                {
                   break;
                }
                continue;
            }
            else
            {
                if(active.track>currTrackPos)
                {
                    currTrackPos++;
                    direction=true;
                    totalTrackMovement++;
                }
                else
                {
                    currTrackPos--;
                    direction=false;
                    totalTrackMovement++;
                }
            }
        } 


     totalTime++;      
    }

}


void parseInputs(int argc ,char** argv)
{
    int c;
    while((c = getopt(argc, argv, "s:")) != -1)
    {
        switch(c){
            case 's':{
                 char z=optarg[0];
                 if(z=='N')
                 {
                    algo= new FIFO();
                    break;
                 }

                 if(z=='S')
                 {
                    algo= new SSTF();
                    break;
                 }
                 if(z=='L')
                 {
                    algo= new LOOK();
                    break;
                 }
                 if(z=='C')
                 {
                    algo= new CLOOK();
                    break;
                 }
                 if(z=='F')
                 {
                    algo= new FLOOK();
                    break;
                 }
            }
        }
    }
}

int main(int argc ,char** argv)
{
     parseInputs(argc,argv);
    inputFile = argv[optind];
    input.open(inputFile);
    if(input.is_open())
    {
        while(getline(input,line)){
            if (line.empty() || line[0] == '#') continue;
            int timeStep;
            int track;
            sscanf(line.c_str(), "%d %d", &timeStep, &track);
            inst i= {timeStep,track};
            instructionQueue.push(i);            
        }
    }
    size=instructionQueue.size();
    startSimulation();
    int waitTime=0;
    int i=0;
    int size=completedQ.size();
    sort(completedQ.begin(),completedQ.end(),compareByTimeStep);
    while(!completedQ.empty())
    {
        inst k=completedQ.front();
        completedQ.pop_front();
        printf("%5d: %5d %5d %5d\n", i, k.timeStep, k.startTime, k.endTime);
         avg_turnaround = avg_turnaround + k.endTime - k.timeStep;
         waitTime= k.startTime -k.timeStep;
         avg_waittime = avg_waittime + k.startTime -k.timeStep;
          if (waitTime > max_waittime)
            max_waittime = waitTime;
        i++;
    }
   avg_turnaround = avg_turnaround / size;
   avg_waittime=avg_waittime/size;
   float ioU = static_cast<float>(totalTrackMovement) / totalTime;
  printf("SUM: %d %d %.4lf %.2lf %.2lf %d\n", totalTime, totalTrackMovement,ioU, avg_turnaround, avg_waittime, max_waittime);

    return 0;
}