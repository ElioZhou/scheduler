#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <queue>
#include <unistd.h>
#include <filesystem>

using namespace std;

vector<int> randvec;
vector<string> desvec;

int ofs = 0;
int quantum;
int maxprios = 4;
const int TRANS_TO_READY = 1;
const int TRANS_TO_RUN = 2;
const int TRANS_TO_BLOCK = 3;
const int TRANS_TO_PREEMPT = 4;
const int TRANS_TO_FINISHED = 5;
bool CALL_SCHEDULER = false;
bool verbose = false;
string scheduler_type;

class Process {
public:
    int AT{};//arrival time
    int TC{};//Total duration of CPU time this process requires
    int CB{};//CPU Burst
    int IO{};//IO Burst
    int pid;
    int transition{};
    int FT{};//Finishing time
    int TT{};//Turnaround time (FT-AT)
    int IT{};//IO time (time in blocked state)
    int CW{};// CPU wait time (time in Ready state)
    int static_priority{};//Static priority
    int dynamic_priority = static_priority - 1;
    int RT{};//Remaining execution time
    int CBRT{};//Remaining CPU Burst
    int inReady{};//The time point when this process transferred to READY
    explicit Process(int id) {
        pid = id;
    }
    void set_priority(int sp) {
        static_priority = sp;
        dynamic_priority = sp - 1;
    }
};

class Event {
public:
    Process* evtProcess;
    int evtTimeStamp;
    int transition;
    int creationTime;
    string oldState;
    string newState;
    void init(int time_stamp, int trans, Process* p, int creation_time) {
        evtProcess = p;
        evtTimeStamp = time_stamp;
        transition = trans;
        creationTime = creation_time;
    }
};

class DES {
//    （1）如果你需要高效的随即存取，而不在乎插入和删除的效率，使用vector
//    （2）如果你需要大量的插入和删除，而不关心随机存取，则应使用list
//    （3）如果你需要随机存取，而且关心两端数据的插入和删除，则应使用deque
public:
    deque<Event> evtQueue;

    Event* get_event() {
        if(evtQueue.empty()) {
            return nullptr;
        }
        Event* evt = &evtQueue.front();
        evtQueue.pop_front();
        return evt;
    }

    void put_event(Event evt) {
        auto itr = evtQueue.begin();
        while(itr != evtQueue.end() && (itr)->evtTimeStamp <= evt.evtTimeStamp) {
            itr++;
        }
        evtQueue.insert(itr, evt);
    }


    int get_next_event_time() {
        if(evtQueue.empty())
            return -1;
        return evtQueue.front().evtTimeStamp;
    }
};

//class DES {
////    （1）如果你需要高效的随即存取，而不在乎插入和删除的效率，使用vector
////    （2）如果你需要大量的插入和删除，而不关心随机存取，则应使用list
////    （3）如果你需要随机存取，而且关心两端数据的插入和删除，则应使用deque
//public:
//    list<Event*> evtQueue;
//
//    Event* get_event() {
//        if(evtQueue.empty()) {
//            return nullptr;
//        }
//        Event* evt = evtQueue.front();
//        evtQueue.pop_front();
//        return evt;
//    }
//
//    void put_event(Event evt) {
//        auto itr = evtQueue.begin();
//        while(itr != evtQueue.end() && (*itr)->evtTimeStamp <= evt.evtTimeStamp) {
//            itr++;
//        }
//        evtQueue.insert(itr, &evt);
//    }
//
//
//    int get_next_event_time() {
//        if(evtQueue.empty())
//            return -1;
//        return evtQueue.front()->evtTimeStamp;
//    }
//};

DES des_layer;

class testDES {
//    （1）如果你需要高效的随即存取，而不在乎插入和删除的效率，使用vector
//    （2）如果你需要大量的插入和删除，而不关心随机存取，则应使用list
//    （3）如果你需要随机存取，而且关心两端数据的插入和删除，则应使用deque
public:
    list<string> evtQueue;

    string get_event() {
        if(evtQueue.empty()) {
            return "";
        }
        string evt = evtQueue.front();
        evtQueue.pop_front();
        return evt;
    }

    void put_event(string evt) {
        auto itr = evtQueue.begin();
        while(itr != evtQueue.end() && itr->substr(2,3) <= evt.substr(2,3)) {
            itr++;
        }
        evtQueue.insert(itr,evt);
    }

    void rm_event() {

    }
};

class Scheduler{
public:
    virtual void add_process(Process *p) = 0;
    virtual Process *get_next_process() = 0;
};

Scheduler *scheduler;

class FCFS : public Scheduler {
public:
    list<Process*> runQueue;

    void add_process(Process* proc) {
        runQueue.push_back(proc);
    }

    Process* get_next_process() {
        if(runQueue.empty())
            return nullptr;
        Process* proc = runQueue.front();
        runQueue.pop_front();
        return proc;
    }

};

class LCFS : public Scheduler {
public:
    list<Process*> runQueue;

    void add_process(Process* proc) {
        runQueue.push_back(proc);
    }

    Process* get_next_process()  {
        if(runQueue.empty())
            return nullptr;
        Process* proc = runQueue.back();
        runQueue.pop_back();
        return proc;
    }
};

class SRTF : public Scheduler {
public:
    list<Process*> runQueue;

    void add_process(Process* proc) {
        auto itr = runQueue.begin();
        while(itr != runQueue.end() && (*itr)->RT <= proc->RT) {
            itr++;
        }
        runQueue.insert(itr, proc);
    }

    Process* get_next_process() {
        if(runQueue.empty())
            return nullptr;
        Process* proc = runQueue.front();
        runQueue.pop_front();
        return proc;
    }
};

class RR : public Scheduler {
public:
    list<Process*> runQueue;

    void add_process(Process* proc) {
//        auto itr = runQueue.begin();
//        while(itr != runQueue.end() && (*itr)->RT <= proc->RT) {
//            itr++;
//        }
        runQueue.push_back(proc);
    }

    Process* get_next_process() {
        if(runQueue.empty())
            return nullptr;
        Process* proc = runQueue.front();
        runQueue.pop_front();
        return proc;
    }
};

//class PRIO : public Scheduler {
//public:
//    queue<Process*> *activeQ = new queue<Process*> [maxprios];
//    queue<Process*> *expiredQ = new queue<Process*> [maxprios];
//
////    queue<Process*>:iterator it = activeQ.begin();
//
//    void add_process(Process* proc) {
//        if(proc->dynamic_priority < 0) {
//            //Modify dynamic priority and insert into expiredQ
//            proc->dynamic_priority = proc->static_priority - 1;
//            expiredQ[proc->static_priority - 1].push(proc);
//        }
//        else {
//            activeQ[proc->dynamic_priority].push(proc);
//        }
//    }
//
//    Process* get_next_process() {
//        //get the first not empty queue
//        queue<Process*> *first_nonempty_queue = first_nonempty(activeQ);
//        if(first_nonempty_queue) {
//            Process* proc = first_nonempty_queue->front();
//            //After running the process, dynamic_priority --
//            proc->dynamic_priority --;
//            first_nonempty_queue->pop();
//            return proc;
//        }
//            //Both activeQ and expiredQ are empty
//        else if(!first_nonempty(expiredQ)){
//            return nullptr;
//        }
//            //activeQ is empty, expiredQ is not empty
//        else {
//            //swap activeQ and expiredQ pointers
//            auto temp = activeQ;
//            activeQ = expiredQ;
//            expiredQ = temp;
//            return get_next_process();
//        }
//    }
//
//    queue<Process*>* first_nonempty(queue<Process*> *queue_ptr) {
//        for(int i = maxprios - 1; i >= 0; i--) {
////            auto kkk = queue_ptr[i];
//            if(!(*(queue_ptr+i)).empty()) {
////                auto its =  queue_ptr[i].front();
//                return &queue_ptr[i];
//            }
//        }
//
//        return nullptr;
//    }
//};
class PRIO : public Scheduler {
public:
    queue<Process*> *activeQ = new queue<Process*> [maxprios];
    queue<Process*> *expiredQ = new queue<Process*> [maxprios];

    void add_process(Process* proc) {
        if(proc->dynamic_priority < 0) {
            //Modify dynamic priority and insert into expiredQ
            proc->dynamic_priority = proc->static_priority - 1;
            expiredQ[proc->static_priority - 1].push(proc);
        }
        else {
            activeQ[proc->dynamic_priority].push(proc);
        }
    }

    Process* get_next_process() {
        //get the first not empty queue
        queue<Process*> *first_nonempty_queue = first_nonempty(activeQ);
        if(first_nonempty_queue) {
            Process* proc = first_nonempty_queue->front();
            //After running the process, dynamic_priority --
//            proc->dynamic_priority --;
            first_nonempty_queue->pop();
            return proc;
        }
            //Both activeQ and expiredQ are empty
        else if(!first_nonempty(expiredQ)){
            return nullptr;
        }
            //activeQ is empty, expiredQ is not empty
        else {
            //swap activeQ and expiredQ pointers
            auto temp = activeQ;
            activeQ = expiredQ;
            expiredQ = temp;
            return get_next_process();
        }
    }

    queue<Process*>* first_nonempty(queue<Process*> *queue_ptr) {
        for(int i = maxprios - 1; i >= 0; i--) {
            if(!(*(queue_ptr+i)).size() == 0) {
                return &queue_ptr[i];
            }
        }
        return nullptr;
    }
};

//class PRIO : public Scheduler {
//public:
////    deque<Process*> *activeQ = new deque<Process*> [maxprios];
////    deque<Process*> *expiredQ = new deque<Process*> [maxprios];
//    vector<deque<Process*>*> activeQ;
//    vector<deque<Process*>*> expiredQ;
//    vector<int> a;
//
//    PRIO(){
//        for (int i = 0; i < maxprios; i++) {
//////            a.push_back(1);
//////            deque<Process*> b = new deque<Process*>();
//            activeQ.push_back(new deque<Process*>);
//            expiredQ.push_back(new deque<Process*>);
//            cout << i;
//        }
//        cout << maxprios;
//        cout << "size" << activeQ.size() << endl;
//    }
////    std::vector<int> myvector (10);
//
////    queue<Process*>:iterator it = activeQ.begin();
//
//    void add_process(Process* proc) {
////        int prio = proc->dynamic_priority;
//        if(proc->dynamic_priority < 0) {
//            //Modify dynamic priority and insert into expiredQ
////            prio = proc->static_priority-1;
////            proc->dynamic_priority = prio;
////            expiredQ[prio].push_back(proc);
//            proc->dynamic_priority = proc->static_priority - 1;
//            expiredQ[proc->static_priority - 1]->push_back(proc);
//        }
//        else {
////            proc->dynamic_priority--;
////            activeQ[prio].push_back(proc)->
//            activeQ[proc->dynamic_priority]->push_back(proc);
////            cout << activeQ[proc->dynamic_priority]->front()->pid << endl;
//        }
//
//    }
//
//    Process* get_next_process() {
//        //get the first not empty queue
//        deque<Process*> *first_nonempty_queue = first_nonempty(activeQ);
//        if(first_nonempty_queue) {
//            Process* proc = first_nonempty_queue->front();
//            //After running the process, dynamic_priority --
////            proc->dynamic_priority --;
//            first_nonempty_queue->pop_front();
//            return proc;
//        }
//        //Both activeQ and expiredQ are empty
//        else if(!first_nonempty(expiredQ)){
//            return nullptr;
//        }
//        //activeQ is empty, expiredQ is not empty
//        else {
//            //swap activeQ and expiredQ pointers
//            auto temp = activeQ;
//            activeQ = expiredQ;
//            expiredQ = temp;
//            return get_next_process();
//        }
//    }
//
//    deque<Process*>* first_nonempty(vector<deque<Process*>*> queue_ptr) {
//        // for loop will work
//        for(int i = maxprios - 1; i >= 0; i--) {
//            if(!queue_ptr[i]->empty()) {
////                auto its =  queue_ptr[i].front();
//                return queue_ptr[i];
//            }
//        }
////        deque<Process*>* aaa = queue_ptr[0];
////        for(auto x : queue_ptr) {
////            if(!x->empty()) {
//////                cout << x->front()->dynamic_priority << endl;
////                if (!aaa->empty() && x->front()->dynamic_priority > aaa->front()->dynamic_priority) {
////                    aaa = x;
////                } else if(aaa->empty()) {
////                    aaa = x;
////                }
////            }
////        }
//        // Reversed iterator will work.
////        std::vector<deque<Process*>*>::reverse_iterator it = queue_ptr.rbegin();
////        while (it != queue_ptr.rend())
////        {
////            if(!(*it)->empty()) {
////                return *it;
////            }
////            it++;
////        }
//        // iterator will not work,SEGFAULT
////        std::vector<deque<Process*>*>::iterator it = queue_ptr.end();
////        while (it != queue_ptr.begin())
////        {
////            if(!(*it)->empty()) {
////                return *it;
////            }
////            it--;
////        }
//        return nullptr;
////        if(aaa->empty()) return nullptr;
////        else return aaa;
//    }
//};

//class PRIO : public Scheduler {
//public:
//    list<Process*> *activeQ = new list<Process*> [maxprios];
//    list<Process*> *expiredQ = new list<Process*> [maxprios];
//
//    void add_process(Process* proc) {
//        if(proc->dynamic_priority < 0) {
//            //Modify dynamic priority and insert into expiredQ
//            proc->dynamic_priority = proc->static_priority - 1;
//            expiredQ[proc->static_priority - 1].push_back(proc);
//        }
//        else {
//            auto list_tmp = activeQ[proc->dynamic_priority];
//            list_tmp.push_back(proc);
//        }
//    }
//
//    Process* get_next_process() {
//        //get the first not empty queue
//        list<Process*> *first_nonempty_queue = first_nonempty(activeQ);
//        if(first_nonempty_queue) {
//            Process* proc = first_nonempty_queue->front();
//            //After running the process, dynamic_priority --
//            proc->dynamic_priority --;
//            first_nonempty_queue->pop_front();
//            return proc;
//        }
//            //Both activeQ and expiredQ are empty
//        else if(!first_nonempty(expiredQ)){
//            return nullptr;
//        }
//            //activeQ is empty, expiredQ is not empty
//        else {
//            //swap activeQ and expiredQ pointers
//            auto temp = activeQ;
//            activeQ = expiredQ;
//            expiredQ = temp;
//            return get_next_process();
//        }
//    }
//
//    list<Process*>* first_nonempty(list<Process*> *queue_ptr) {
//        for(int i = maxprios - 1; i >= 0; i--) {
////            auto kkk = queue_ptr[i];
//            if(!(*(queue_ptr+i)).size() == 0) {
////                auto its =  queue_ptr[i].front();
//                return &queue_ptr[i];
//            }
//        }
//
//        return nullptr;
//    }
//};

//class PREPRIO : public Scheduler {
//public:
//    queue<Process*> *activeQ = new queue<Process*> [maxprios];
//    queue<Process*> *expiredQ = new queue<Process*> [maxprios];
//
//    void add_process(Process* proc) {
//        if(proc->dynamic_priority < 0) {
//            //Modify dynamic priority and insert into expiredQ
//            proc->dynamic_priority = proc->static_priority - 1;
//            expiredQ[proc->static_priority - 1].push(proc);
//        }
//        else {
//
//        }
//    }
//
//    Process* get_next_process() {
//        //get the first not empty queue
//        auto first_nonempty_queue = first_nonempty(activeQ);
//        if(first_nonempty_queue) {
//            Process* proc = first_nonempty_queue->front();
//            first_nonempty_queue->pop();
//        }
//            //activeQ is empty
//        else {
//            //swap activeQ and expiredQ pointers
//            auto temp = activeQ;
//            activeQ = expiredQ;
//            expiredQ = temp;
//            return get_next_process();
//        }
//    }
//
//    queue<Process*>* first_nonempty(queue<Process*> *queue_ptr) {
//        for(int i = 0; i < maxprios; i++)
//            if(!queue_ptr[i].empty())
//                return queue_ptr + i;
//        return nullptr;
//    }
//};

void des_test() {
    ifstream testFile("/Users/ethan/Documents/NYU/22 Spring/Operating Systems/Labs/Lab2/lab2_assign/des_test");
    string teststr;
    testDES testdes;
    while(getline(testFile, teststr))
        testdes.put_event(teststr);
    string str = testdes.get_event();
    while(str != "") {
        cout << str << endl;
        str = testdes.get_event();
    }
    testFile.close();
}

int myrandom(int burst){
    if(ofs == randvec.size()) {
        ofs = 0;
    } else {
        ofs++;
    }
    return 1 + (randvec[ofs] % burst);
}

void Simulation() {
    int oldStatePeriod;
    Event* current_event;
    // Keep track of what process is running now
    Process* current_running_process = nullptr;

    // conceptually converted to bool, nullptr is treated as false
    while((current_event = des_layer.get_event())) {
        int current_time = current_event->evtTimeStamp;
        int transition = current_event->transition;
        Process* process_in_event = current_event->evtProcess;
//        no need to delete, the destructor will be called when this loop terminates
//        delete current_event;

        switch(transition) {
            case TRANS_TO_READY: {
                current_event->newState = "READY";
                oldStatePeriod = current_time - current_event->creationTime;
                scheduler->add_process(process_in_event);
                process_in_event->inReady = current_time;
//                cout << "Process " << process_in_event->pid << " is ready in " << current_time << endl;
                // call scheduler
                CALL_SCHEDULER = true;
                if(verbose) {
                    cout << current_time << " " << process_in_event->pid << " " ;
                    cout << oldStatePeriod << ": " << current_event->oldState << " -> " << current_event->newState << endl;
                }
                break;
            }
            case TRANS_TO_RUN: {
//                One trick to deal with schedulers is to treat non-preemptive scheduler as preemptive with very large
//                quantum that will never fire (10K is good for our simulation).
                current_event->newState = "RUNNING";
                oldStatePeriod = current_time - process_in_event->inReady;
                //Set current running process
                current_running_process = process_in_event;

                //-----------------Generate CPU burst-------------------//

                //If remaining CPU burst is not zero, execute the remaining CPU burst, and not generate new one
                int CPU_burst;
                if(current_running_process->CBRT > 0)
                    CPU_burst = current_running_process->CBRT;
                else CPU_burst = myrandom(current_running_process->CB);
                CPU_burst = min(current_running_process->RT, CPU_burst);
                current_running_process->CBRT = CPU_burst;
                if (verbose) {
                    cout<< current_time<<" "<< current_running_process->pid <<" "<< oldStatePeriod <<": "<< current_event->oldState <<" -> "<< current_event->newState;
                    cout <<" cb="<< CPU_burst <<" rem="<< current_running_process->RT << " prio="<< current_running_process->dynamic_priority << endl;
                }

                //-----------------Run the process-------------------//

                //Define how long the process will run this turn
                int exec_time{};
                exec_time = min(CPU_burst, quantum);
                current_running_process->CBRT -= exec_time;
                current_running_process->RT -= exec_time;
                //Define when this execution turn finishes
                int turn_finished_time = exec_time + current_time;
                //The process will be interrupted before the cpu burst exhausted, if quantum is smaller
                //In this case, current_running_process->RT
                if (quantum < CPU_burst) {
                    Event e;
                    e.init(turn_finished_time, TRANS_TO_PREEMPT, current_running_process, current_time);
                    e.oldState = "RUNNING";
                    des_layer.put_event(e);
                }
                //The  process will not be interrupted
                else {
                    //If remaining time == 0, trans to done
                    //else, trans to block
                    if (current_running_process->RT == 0) {
                        Event e;
                        e.init(turn_finished_time, TRANS_TO_FINISHED, current_running_process, current_time);
                        e.oldState = "RUNNING";
                        des_layer.put_event(e);
                    }
                    else {
                        Event e;
                        e.init(turn_finished_time, TRANS_TO_BLOCK, current_running_process, current_time);
                        e.oldState = "RUNNING";
                        des_layer.put_event(e);
                    }
                }
                current_running_process->CW += (current_time - current_running_process->inReady);
//                cout << "Process " << process_in_event->pid << " starts to run in " << current_time << endl;
                break;
            }
            case TRANS_TO_BLOCK: {
                current_event->newState = "BLOCK";
                oldStatePeriod = current_time - current_event->creationTime;
                int IO_burst = myrandom(process_in_event->IO);
                int burst_finished_time = IO_burst + current_time;
                if (verbose) {
//                    current_event->newState = "BLOCK";
                    cout << current_time<<" "<< process_in_event->pid << " "<< oldStatePeriod << ": ";
                    cout << current_event->oldState <<" -> " << current_event->newState;
                    cout << " ib="<< IO_burst <<" rem="<< process_in_event->RT << endl;
                }
                //When IO burst finished, trans to READY
                Event e;
                e.init(burst_finished_time, TRANS_TO_READY, process_in_event, current_time);
                e.oldState = "BLOCK";
                des_layer.put_event(e);
                //No process is running
                current_running_process = nullptr;
                process_in_event->dynamic_priority = process_in_event->static_priority-1;//when a processes returns from I/O, reset dynamic_priority
                process_in_event->IT += IO_burst;
//                cout << "Process " << process_in_event->pid << " is blocked in " << current_time << endl;
                CALL_SCHEDULER = true;

                break;
            }
            case TRANS_TO_PREEMPT: {
                current_event->newState = "PREEMPT";
                oldStatePeriod = current_time - current_event->creationTime;
                current_running_process = nullptr;
                if (verbose) {
                    cout <<current_time<<" "<< process_in_event->pid << " "<< oldStatePeriod << ": " << current_event->oldState <<" -> "<< current_event->newState;
                    cout << " cb=" << process_in_event->CBRT <<" rem="<<process_in_event->RT << " prio="<<process_in_event->dynamic_priority<< endl;
                }
                process_in_event->dynamic_priority --;
                scheduler->add_process(process_in_event);
                process_in_event->inReady = current_time;
//
                CALL_SCHEDULER = true;

//                process_in_event->dynamic_priority --;
                break;
            }
            case TRANS_TO_FINISHED: {
                current_event->newState = "Done";
                oldStatePeriod = current_time - current_event->creationTime;
                current_running_process = nullptr;
                process_in_event->FT = current_time;
                process_in_event->TT = current_time - process_in_event->AT;
//                cout << "Process " << process_in_event->pid << " is finished in " << current_time << endl;
                CALL_SCHEDULER = true;
                if (verbose) {
                    cout << current_time<<" "<< process_in_event->pid <<" "<< oldStatePeriod << ": " << current_event->newState<<endl;
                }
                break;
            }
            default:
                break;
        }
        if(CALL_SCHEDULER) {
            // If there are many event at the same time, process next event from Event Queue
            if(des_layer.get_next_event_time() == current_time)
                continue;
            CALL_SCHEDULER = false;
            if(!current_running_process) {
                current_running_process = scheduler->get_next_process();
                if(!current_running_process)
                    continue;
                // create event to make this process runnable
                Event e;
                e.init(current_time, TRANS_TO_RUN, current_running_process, current_time);
                e.oldState = "READY";
                des_layer.put_event(e);
//                current_running_process = nullptr;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    /* Steps:
     * 1: Read input file
     * 2: Create Process object
     * 3: Put it into event queue
     * 4: In Simulation, get event, and call scheduler according to different types of transition
     */
    vector<Process *> processes;
//    string scheduler_type;


    //---------------------read parameter---------------------//
    int c;
    int count = 0;
    while ((c = getopt (argc, argv, "vteps::")) != -1) {
        count += 1;
        switch (c)
        {
            case 'v':
                verbose = true;
                break;
            case 't':
                //“-t” traces the event execution.
                break;
            case 'e':
                // “-e” shows the eventQ before and after an event is inserted
                break;
            case 'p':
                //“-p” shows for the E scheduler the
                //decision when a unblocked process attempts to preempt a running process.
                //Remember two conditions must be met (higher prio and
                //pending event of the currently running process is in the future, not now
                break;
            case 's':{
                size_t pos = 0;
                string optarg_str = string(optarg);
                if((pos = optarg_str.find (':')) != string::npos) {
                    quantum = stoi(optarg_str.substr(1, pos)); // store the substring
                    optarg_str.erase(0, pos + 1);  //erase() function
                    maxprios = stoi(optarg_str);
                } else if(optarg_str.length() > 1){
                    quantum = stoi(optarg_str.substr(1, optarg_str.length()));
                }
                if (optarg[0] == 'F') {
                    scheduler_type = "FCFS";
                    scheduler = new FCFS();
                    quantum = 100000;
                } else if(optarg[0] == 'L') {
                    scheduler_type = "LCFS";
                    scheduler = new LCFS();
                    quantum = 100000;
                } else if(optarg[0] == 'S') {
                    scheduler_type = "SRTF";
                    scheduler = new SRTF();
                    quantum = 100000;
                } else if(optarg[0] == 'R') {
                    scheduler_type = "RR";
                    scheduler = new RR();
                } else if(optarg[0] == 'P') {
                    scheduler_type = "PRIO";
                    scheduler = new PRIO();
                } else if(optarg[0] == 'E') {

                }
                break;
            }
            default:
                cout << c;
        }
    }

    //---------------------read rfile---------------------//
    ifstream randFile(argv[count + 2]);
//    std::__fs::filesystem::path p = std::__fs::filesystem::current_path();

//    std::cout << "The current path " << p;
//    ifstream randFile("../../lab2_assign/rfile");
//    ifstream randFile("/Users/ethan/Documents/NYU/22 Spring/Operating Systems/Labs/Lab2/lab2_assign/rfile");
    string randstr;
    while (randFile >> randstr)
        randvec.push_back(stoi(randstr, nullptr, 10));
    randFile.close();

    //---------------------Set scheduler---------------------//

//        scheduler = new FCFS();quantum = 10000;scheduler_type = "FCFS";
//        scheduler = new LCFS();quantum = 10000;scheduler_type = "LCFS";
//        scheduler = new SRTF();scheduler_type = "SRTF";quantum = 10000;
//        scheduler = new RR();scheduler_type = "RR";quantum = 2;
//    scheduler = new PRIO(); scheduler_type = "PRIO"; quantum = 50; maxprios = 10;

    //---------------------read input processes file---------------------//
    ifstream inputFile(argv[count + 1]);
//    ifstream inputFile("input0");
//    ifstream inputFile("/Users/ethan/Documents/NYU/22 Spring/Operating Systems/Labs/Lab2/lab2_assign/input3");
    string token;
    int i = 0;
    while (inputFile >> token) {
//        This is not correct, local variables will be deleted when the scope ends.
//        So the process will always be created at the same address, it'll be overwritten.
//        Process process1 {i};
//        cout << &process1 << endl;
        Process *process = new Process(i);
//        process->set_priority(myrandom(maxprios));
        process->set_priority(myrandom(maxprios));

        process->AT = stoi(token, nullptr, 10);

        for (int j = 0; j < 3; j++) {
            inputFile >> token;
            if (j == 0) {
                process->TC = stoi(token, nullptr, 10);
                process->RT = process->TC;
            } else if (j == 1) process->CB = stoi(token, nullptr, 10);
            else process->IO = stoi(token, nullptr, 10);
        }
        //Creat event for the process
        //Pass by value, not pointer, so it's OK, refer to the beginning of the while loop.
        Event e;
        e.init(process->AT, TRANS_TO_READY, process, process->AT);
        e.oldState = "CREATED";
        des_layer.put_event(e);
        processes.push_back(process);
        i++;
    }
    inputFile.close();

    //---------------------Start simulating---------------------//

    Simulation();

    //---------------------Statistics---------------------//

    cout << scheduler_type << endl;
    cout << " pid:   AT   TC   CB   IO PR|    FT    TT    IT    CW" << endl;
    for (int i = 0; i < processes.size(); i++) {
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d", processes[i]->pid,
               processes[i]->AT, processes[i]->TC, processes[i]->CB, processes[i]->IO,
               processes[i]->static_priority, processes[i]->FT, processes[i]->TT,
               processes[i]->IT, processes[i]->CW);
        cout << "   TT = FT - AT: " << processes[i]->TT - (processes[i]->FT - processes[i]->AT) << "  |  ";
        cout << "TT = CW + IT + TC: " << processes[i]->TT - (processes[i]->CW + processes[i]->IT + processes[i]->TC)
             << endl;
    }
//    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", );

        //---------------------DES test---------------------//

//    des_test();
        return 0;
}
