#include <string.h>
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

using namespace std;

enum state {CREATED, READY, RUNNING, BLOCKED, COMPLETE};
enum trans {TRANS_TO_READY, TRANS_TO_PREEMPT, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_COMPLETE};

// Classes
class Process;  
class SchedulerFCFS;
class SchedulerLCFS;
class SchedulerSRTF;
class SchedulerRR;
class SchedulerPriority;
class SchedulerPrePriority;
class Event; 
class DESLayer; 
class Scheduler;

// FUNCTION DECLARATIONS
int getRandom(int burst);
void parseInputs(int argc, char** argv);
void getRandNum();
//void readInputallProcess(); 
void mySimulator();

// PROCESS CLASS 
class Process {  
    public:
        static int count;
        int pid;  
        int AT;
        int TC;  
        int CB; 
        int IO; 
        int static_prio; 
        int dynamic_prio; 
        int ft; 
        int tt; 
        int it; 
        int cw;
        int cb_rem;  
        state p_state; 
        int state_ts; 
        int premflag; 
        int rem_burst; 
        int cb; 
        int ib; 
        int finishFlag; 
        int timePrev;
        
        // constructor
        Process() {
            //cout << "Process created!\n";
            this->pid = count; 
            this->p_state = CREATED;
            //this->cb_rem = this->CB;
            //this->state_ts = this->AT;  
            count++;
        }
        // setter functions 
        void setStaticPrio(int static_prio) {
            this->static_prio = static_prio; 
        }
        void setDynamicPrio(int dynamic_prio) {
            this->dynamic_prio = dynamic_prio; 
        }
        void setState(state p_state) {
            this->p_state = p_state; 
        }
        void setParams(int AT, int TC, int CB, int IO) {
            this->AT = AT;
            this->CB = CB; 
            this->TC = TC; 
            this->IO = IO; 
        }
        void setDerivedParams(int ft, int tt, int it, int cw) {
            this->ft = ft;
            this->tt = tt; 
            this->it = it; 
            this->cw = cw; 
        }
        // getter functions 
        int getAT() {
            return this->AT;  
        }
        int getTC() {
            return this->TC;  
        }
        int getCB() {
            return this->CB;  
        }
        int getIO() {
            return this->IO;  
        }
        int getStaticPrio() {
            return this->static_prio; 
        }
        int getDynamicPrio() {
            return this->dynamic_prio;
        }
        state getState() {
            return this->p_state; 
        }

};

// EVENT CLASS
class Event { 
    public:
        static int ecount; 
        int eid;  
        int eventTimeStamp; 
        Process *eventProcess; 
        trans transition;

        Event() {
            this->eid = ecount;
            ecount++;
        }
};

struct  cmp {
    bool operator()(const Event* e, const Event* f) {
        if(e->eventTimeStamp == f->eventTimeStamp) {
            return e->eid < f->eid;
        }
        return e->eventTimeStamp < f->eventTimeStamp;
    }
};

class DESLayer {
     public: 
         static set<Event*, cmp> EventQueue; 

     public:

        static void putEvent(Event *e) {
            EventQueue.insert(e);
        }

        static Event* get_event() {
            if(!EventQueue.empty()) {
                Event *e = *EventQueue.begin(); 
                return e;
            }
            else {
                return nullptr;
            }
        }

        static void rm_event(Event* e) {
            EventQueue.erase(e);
        }

        static int get_next_event_time() {
            if(!EventQueue.empty()) {
                Event* e = get_event();
                return e->eventTimeStamp;
            }
            else {
                return -1; 
            }
        }
        
        DESLayer() {
        }
}; 

// GLOBAL VARIABLES 
int vflag = 0;
int tflag = 0;
int eflag = 0; 
int pflag = 0; 
string stringOpt = "";
char s; 
string inputFile, randFile;
fstream input, rfile;
vector<int> randvals;
int ofs = 0; 
int Process::count = 0; 
int Event::ecount = 0;
int CURRENT_TIME = 0;
Process* CURRENT_RUNNING_PROCESS = nullptr;
bool CALL_SCHEDULER = false;
set <Event*, cmp> DESLayer::EventQueue;
int size = 0; 
int quant = 10000, maxprio = 4; 
vector<Process *> allProcess; 
DESLayer* mySimulatorLayer = new DESLayer();
Scheduler* scheduler = nullptr;
int finish_event_time = 0;
int total_io = 0;



class Scheduler {
   public:
      // pure virtual function providing interface framework.
      virtual void addProcess(Process* p) = 0;
      virtual Process* getNextProcess() = 0;  
   
};
 
class SchedulerFCFS: public Scheduler {
   public:
      void addProcess(Process* p) {
          runQueue.push(p);
      }

      Process* getNextProcess() {
          Process* p; 
          if(runQueue.empty()) {
             return nullptr;
          }
          else {
              p = runQueue.front();
              runQueue.pop();
              return p;
          }

      }
    protected:
      queue<Process*> runQueue; 
};

class SchedulerLCFS: public Scheduler {
   public:
      void addProcess(Process* p) {
          runQueue.push_back(p);
      }

      Process* getNextProcess() {
          Process* p; 
          if(runQueue.empty()) {
              return nullptr;
          }
          else {
              p = runQueue.back(); 
              runQueue.pop_back();
              return p;
          }

      }
    protected:
      deque<Process*> runQueue; 
};


class SchedulerSRTF: public Scheduler {
   public:
      void addProcess(Process* p) {

        if(!runQueue.empty()){
             auto it = runQueue.begin();
            for(;it!=runQueue.end() && (*it)->cb_rem <= p->cb_rem;) {
                it++; 
            }
            runQueue.insert(it, p);
           } 
        else {
            runQueue.push_back(p);
            return;
            }
       }           

      Process* getNextProcess() {
          Process* p; 
          if(runQueue.empty()) {
              return nullptr;
          }
          else {
              p = runQueue.front();
              runQueue.pop_front();
              return p;
          }

      }

    protected:
      deque<Process*> runQueue; 
};


class SchedulerRR: public Scheduler {
   public:
      void addProcess(Process* p) {
          runQueue.push(p);
      }

      Process* getNextProcess() {
          Process* p; 
          if(runQueue.empty()) {
             return nullptr;     
          }
          else {
              p = runQueue.front();
              runQueue.pop();
              return p;
          }

      }

    protected:
      queue<Process*> runQueue; 
};

class SchedulerPriority: public Scheduler {
   public:
      
      SchedulerPriority(int max) {
        maxprio = max; 
        activeQueue.resize(max);
        expiredQueue.resize(max);
      }

      void addProcess(Process* p) {
          if(p->dynamic_prio == -1) {
            p->dynamic_prio = p->static_prio-1;
            expiredQueue[p->dynamic_prio].push_back(p);
          }
          else {
            activeQueue[p->dynamic_prio].push_back(p);
          }
      }

      int isEmpty(vector<deque<Process *> > p) {
        for(int i=0; i< maxprio; ++i) {
            if(p[i].empty()) {
                continue; 
            }
            else {
                return 0; 
            }
        }
        return 1; 
      }

      Process* getNextProcess() {
        if(isEmpty(activeQueue) && !(isEmpty(expiredQueue)))
        {
             activeQueue.swap(expiredQueue);
        }

        for(int i=maxprio-1; i>=0; i--) {
            if(!activeQueue[i].empty()){
                Process* p = activeQueue[i].front(); 
                activeQueue[i].pop_front();
                return p; 
            }
            else
                continue;
        }
        return nullptr;
      }

    protected:
      vector<deque<Process*> > activeQueue; 
      vector<deque<Process*> > expiredQueue; 
      int maxprio; 
};

class SchedulerPrePriority: public Scheduler {
   public:
      
      SchedulerPrePriority(int max) {
        maxprio = max; 
        activeQueue.resize(max);
        expiredQueue.resize(max);
      }

      void addProcess(Process* p) {
          if(p->dynamic_prio == -1) {
            p->dynamic_prio = p->static_prio-1;
            expiredQueue[p->dynamic_prio].push_back(p);
          }
          else {
            activeQueue[p->dynamic_prio].push_back(p);
          }
      }

      int isEmpty(vector<deque<Process *> > p) {
        for(int i=0; i< maxprio; ++i) {
            if(p[i].empty()) {
                continue; 
            }
            else {
                return 0; 
            }
        }
        return 1; 
      }

      Process* getNextProcess() {
        if(isEmpty(activeQueue))
        {
            if(!(isEmpty(expiredQueue)))
            {
                if(isEmpty(activeQueue)) {
                    activeQueue.swap(expiredQueue);
                }
            }
        }

                for(int i=maxprio-1; i>=0; i--) {
                    if(!activeQueue[i].empty()){
                        Process* p = activeQueue[i].front(); 
                        activeQueue[i].pop_front();
                        return p; 
                    }
                    else
                        continue;
                }
        return nullptr;
      }

    protected:
      vector<deque<Process*> > activeQueue; 
      vector<deque<Process*> > expiredQueue; 
      int maxprio; 
};


void mySimulator() {
         Event* evt;
         int allProcess_blocked = 0;
         int  block_time = 0;
         while( (evt = mySimulatorLayer->get_event()) ) {
                 Event* newEvent;
                 Process *cProc = evt->eventProcess;  
                 CURRENT_TIME = evt->eventTimeStamp;
                 trans transition = evt->transition;
                 cProc->timePrev = CURRENT_TIME-cProc->state_ts;
                 cProc->state_ts = evt->eventTimeStamp;
                 mySimulatorLayer->rm_event(evt); 
                 delete evt;                  
                 evt = nullptr;
                

                 switch(transition) {  
                    case TRANS_TO_READY: 
                        if(CURRENT_RUNNING_PROCESS != nullptr && s == 'E') {
                            if(cProc->dynamic_prio > CURRENT_RUNNING_PROCESS->dynamic_prio) {
                                for(auto it = mySimulatorLayer->EventQueue.begin(); it != mySimulatorLayer->EventQueue.end(); ++it) {
                                    if((*it)->eventProcess->pid == CURRENT_RUNNING_PROCESS->pid && (*it)->eventTimeStamp != CURRENT_TIME) {
                                        CURRENT_RUNNING_PROCESS->rem_burst = CURRENT_RUNNING_PROCESS->rem_burst + (*it)->eventTimeStamp - CURRENT_TIME;
                                        mySimulatorLayer->rm_event(*it); 
                                        Event* preprio = new Event(); 
                                        preprio->eventProcess = CURRENT_RUNNING_PROCESS; 
                                        preprio->eventTimeStamp = CURRENT_TIME; 
                                        preprio->transition = TRANS_TO_PREEMPT; 
                                        mySimulatorLayer->putEvent(preprio); 
                                        CURRENT_RUNNING_PROCESS = nullptr; 
                                        break; 

                                    }
                                     
                                }
                            }
                        }

                        if(cProc->p_state == BLOCKED) {
                            cProc->it = cProc->it + cProc->timePrev;
                            allProcess_blocked--;
                            if(allProcess_blocked==0)
                                total_io = total_io+ CURRENT_TIME - block_time;
                        }
                        scheduler->addProcess(cProc);
                        CALL_SCHEDULER = true;
                        cProc->p_state = READY;
                        break;

                    case TRANS_TO_RUN:
                         CURRENT_RUNNING_PROCESS = cProc;
                         cProc->cw = cProc->cw + cProc->timePrev;

                        if(cProc->rem_burst == 0)
                            cProc->cb = getRandom(cProc->CB);
                        else
                            cProc->cb = cProc->rem_burst;
                        
                        cProc->cb = min(cProc->cb_rem, cProc->cb);
                        if(cProc->cb > quant) {
                            cProc->rem_burst = cProc->cb - quant; 
                            newEvent = new Event();
                            newEvent->eventTimeStamp = CURRENT_TIME + quant;
                            newEvent->transition = TRANS_TO_PREEMPT; 

                        }
                        else
                        {
                            if(cProc->cb == cProc->cb_rem) {
                                newEvent = new Event(); 
                                newEvent->eventTimeStamp = CURRENT_TIME + cProc->cb_rem;
                                newEvent->transition = TRANS_TO_COMPLETE;  
    
                            }
                            else 
                            {
                                newEvent = new Event(); 
                                newEvent->eventTimeStamp = CURRENT_TIME + cProc->cb;
                                newEvent->transition = TRANS_TO_BLOCK;
                                cProc->rem_burst = 0;                           
                            }
                        }
                        cProc->p_state = RUNNING;
                        newEvent->eventProcess = cProc;
                        mySimulatorLayer->putEvent(newEvent);
                        break;

                    
                    case TRANS_TO_PREEMPT: 

                        CURRENT_RUNNING_PROCESS = nullptr;
                        cProc->dynamic_prio = cProc->dynamic_prio-1;
                        scheduler->addProcess(cProc);
                        cProc->p_state = READY;
                        CALL_SCHEDULER = true;
                        cProc->cb_rem = cProc->cb_rem - cProc->timePrev;
                        cProc->cb = cProc->cb - min(quant, cProc->timePrev);

                        break;


                    case TRANS_TO_BLOCK:
                        allProcess_blocked = allProcess_blocked + 1; 
                        if(allProcess_blocked == 1) {
                            block_time = CURRENT_TIME;
                        }
                        cProc->cb_rem = cProc->cb_rem - cProc->timePrev;
                        newEvent = new Event();                 
                        newEvent->eventProcess = cProc;
                        cProc->ib = getRandom(cProc->IO);   
                        newEvent->transition = TRANS_TO_READY;
                        newEvent->eventTimeStamp = CURRENT_TIME + cProc->ib;
                        cProc->p_state = BLOCKED;    
                        mySimulatorLayer->putEvent(newEvent);
                        cProc->dynamic_prio = cProc->static_prio-1;
                        CURRENT_RUNNING_PROCESS = nullptr;
                        CALL_SCHEDULER = true;
                        break;

                    case TRANS_TO_COMPLETE: 
                        cProc->ft = CURRENT_TIME; 
                        cProc->tt = cProc->ft - cProc->AT;
                        cProc->p_state = COMPLETE;
                        CALL_SCHEDULER = true; 
                        CURRENT_RUNNING_PROCESS = nullptr;
                        finish_event_time = CURRENT_TIME;
                        break;
                }
                 if(CALL_SCHEDULER) {
                         if (mySimulatorLayer->get_next_event_time() == CURRENT_TIME) {
                                continue;       
                         }
                         CALL_SCHEDULER = false; 
                         if (CURRENT_RUNNING_PROCESS == nullptr) {
                            CURRENT_RUNNING_PROCESS = scheduler->getNextProcess();
                                   if (CURRENT_RUNNING_PROCESS == nullptr) {  
                                       continue;
                                   }
                                    newEvent = new Event(); 
                                    newEvent->eventTimeStamp = CURRENT_TIME;
                                    newEvent->eventProcess = CURRENT_RUNNING_PROCESS; 
                                    newEvent->transition = TRANS_TO_RUN;
                                    cProc->state_ts = CURRENT_TIME;
                                    mySimulatorLayer->putEvent(newEvent); 
                            }
                 }
                 newEvent = nullptr;
         }
}


void getRandNum() {
    string nums = "\0";
    rfile.open(randFile);
    if(rfile.is_open()) {
        getline(rfile, nums);
        size = stoi(nums);
        for(int i = 0; getline(rfile, nums); i++) {
                randvals.push_back(stoi(nums));
        }
    }
    rfile.close();
}


int getRandom(int burst) { 
    if(ofs == size) {
        ofs = 0;
    }
    return 1 + (randvals[ofs++] % burst);
}


// MAIN FUNCTION
int main(int argc, char** argv) {
    parseInputs(argc, argv); 
    string procParams;
    getRandNum();
    char *token = nullptr;
    int i=0; 
    int* param = (int *) malloc(sizeof(int)*5);
    
    input.open(inputFile);
    if(input.is_open()) {
        while(true) {
            getline(input, procParams);
            if(input.eof()) {
               break; 
            }
            i = 0;
            char *params = (char *) procParams.c_str(); 
            token = strtok(params, " ");
            while(token!=nullptr) {
                param[i++] = stoi(token);
                token = strtok(nullptr, " ");
            }
            Process* p = new Process();
            allProcess.push_back(p);
            Event* e = new Event();
            param[i] = getRandom(maxprio); 
            int parameter1 =param[0];  int parameter2 =param[1];  int parameter3 =param[2];  int parameter4 =param[3];
            p->setParams(parameter1, parameter2, parameter3, parameter4);
            int staticpriority=param[4];
            p->setStaticPrio(staticpriority);
            p->setDynamicPrio(staticpriority-1);
            p->cb_rem = p->TC;
            p->state_ts = p->AT;
            p->cw = 0;
            p->it = 0;
            p->ft = 0;
            p->tt = 0;
            p->rem_burst = 0;
            p->premflag = 0;
            e->eventProcess = p;
            e->eventTimeStamp = p->AT;
            e->transition = TRANS_TO_READY; 
            mySimulatorLayer->putEvent(e); 
        }
    }
    input.close();

	if (s=='F') {
		scheduler = new SchedulerFCFS();
	}
    else if (s == 'S') {
		scheduler = new SchedulerSRTF();
	}
    else if (s =='R') {
		scheduler = new SchedulerRR();
	}
	else if (s == 'L') {
		scheduler = new SchedulerLCFS();
	}
	else if (s =='R') {
		scheduler = new SchedulerRR();
	}
	else if (s =='P') {
		scheduler = new SchedulerPriority(maxprio);
	}
    else if (s =='E') {
		scheduler = new SchedulerPrePriority(maxprio);
	}

    mySimulator();
    int total_tt = 0, total_cw = 0, total_tc = 0;

    if (s=='F') {
		cout<<"FCFS\n";
	}
    else if (s == 'S') {
		cout<<"SRTF\n";
	}
    else if (s =='R') {
		cout<<"RR " <<quant<<endl;
	}
	else if (s == 'L') {
		cout<<"LCFS\n";
	}
	else if (s =='P') {
		cout<<"PRIO "<<quant<<endl;
	}
    else if (s =='E') {
		cout<<"PREPRIO "<<quant<<endl;
	} 

    for(auto it= allProcess.begin(); it!= allProcess.end(); ++it) {
        Process *p = *it; 
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
                p->pid, p->AT, p->TC, p->CB, p->IO, p->static_prio, p->ft, p->tt, p->it, p->cw);
        
        total_tt = total_tt + p->ft - p->AT;
        total_tc = total_tc + p->TC;
        total_cw = total_cw + p->cw;
    }
    double cpu_util = (double)(total_tc*100.0/finish_event_time);
    double io_util = (double)(total_io*100.0/finish_event_time);
    double avg_tt = (double)(total_tt*1.0/allProcess.size());
    double avg_cw = (double)(total_cw*1.0/allProcess.size());
    double throughput = (double)(allProcess.size()*100.0/finish_event_time);

    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", finish_event_time, cpu_util, io_util, avg_tt, avg_cw, throughput);
    randvals.clear();
    delete(mySimulatorLayer);
 }

void parseInputs(int argc, char** argv) { 
    int c;  
    while((c = getopt(argc, argv, "vtpes:")) != -1) {
        switch(c) {
             case 't':
                    tflag = 1;
                    break; 
            case 'v':
                    vflag = 1;
                    break;
            
            case 'e':
                eflag = 1;
                break;

            case 'p':
                    pflag = 1;
                    break;
            case 's': 
                    stringOpt = optarg;
                    break;
        }
    }
    
    sscanf(stringOpt.c_str(), "%c%d:%d", &s, &quant, &maxprio);

    inputFile = argv[optind];
    randFile = argv[optind+1];

}



