# Bus-Station-Simulation
#### Assignment 3 for lesson: [`K22 Operating System`](http://cgi.di.uoa.gr/~ad/k22/) (DI University of Athens)
Task
------------
Bus station simulation using shared memory and POSIX semaphores, more info:
[`Project 3 for Winter Semester 2019-2020`](http://cgi.di.uoa.gr/~ad/k22/OS-F19-Prj3.pdf)

### Diagram for the operation of the station:<br />
![bss_simulation](https://user-images.githubusercontent.com/26937033/87961182-0b46ae80-cabe-11ea-9678-90124d2ccd63.png)<br />

### Behavior of processes:<br />

BUS PROCESS:
1) Arrives and waits for service from station-manager(entry-request).
2) Updates from station-manager for safe entrance to the station on a specific position.
3) Arrives on the specific position & disembarks the passengers.
4) Updates ledger for the number of arrivals.
5) Waits in the parking lot & boards passengers.
6) Makes request to depart and waits for service from station-manager(exit-request).
7) Updates from station-manager for safe departure.
8) Departs and enters the public road.

STATION-MANAGER:
1) Receives entry-requests from buses. If there is at most one outgoing bus, accepts an entry request from a bus and gives the order for parking in an empty position(Priority: VOR, ASK, PEL).
2) Receives exit-requests from buses. If there is at most one ingoing bus, accepts an exit request from a bus and gives the order for departure.
3) In 1) and 2) updates the ledger for arrival/departure time, bus type etc.
4) At any time there is at most one outgoing and ingoing bus in the station.

COMPTROLLER:
1) Periodically provides the status of the station(number of parked buses, empty positions, number of passengers).
2) Periodically prints statistics(average wait, waiting by bus type, etc.)

Implementation
--------------
*****Structure of config.csv***** <br />
(name_for_logfile)<br />
(time for comptroller) (stattimes for comptroller)<br />
(bus_type) (busses_of_that_type) -> x 3 for all types<br />
(bus_type) (passengers) (passengers_capacity) (parkperiod) (mantime) -> x busses_of_that_type for all types<br />

### Communication protocol of processes:<br />

**BUS-ARRIVING**
```
    P(change_reqs);
    requests++;
    V(change_reqs);

    P(request_in);
    flag_in=true;
    bus_type=ASK;
    V(ready);
    P(bus_in);
    V(request_in);

    P(change_reqs);
    requests--;
    V(change_reqs);
```

**BUS-IN STATION**
```
    P(change_reqs);
    buses++;
    V(change_reqs);
```

**BUS-LEAVING**
```   
    P(change_reqs);
    requests++;
    V(change_reqs);

    P(request_out);
    flag_out=true;
    V(ready);
    P(bus_out);
    V(request_out);

    P(change_reqs);
    requests--;
    buses--;
    V(change_reqs);
```

**STATION-MANAGER**
```   
    while(exiting){
        ...
        if((requests>0) && (buses>0)){
            if(!waiting) P(ready);
                if(flag_in){
                    if(enough capacity){
                        position = given_position;
                        flag_in = false;
                        waiting = false;
                        V(bus_in);
                    }
                    else waiting = true;
                }
                if(waiting && buses>0) P(ready);
                if(flag_out){
                    flag_out = false;
                    V(bus_out);
                }
        }
        else exiting = false;
    }
```

Separate program execution
------------
``` 
->mystation.c = the main program with flags: -l config.csv -e YES/NO -c YES/NO
  { 
    -e YES=automatic execution of bus program (from mystation with fork & exec), NO=manual program execution of bus program 
    -c YES=delete the logfile at the end of the program, NO=no logfile deletion
  }
->bus.c = flags: -t type -n incpassengers -c capacity -p parkperiod -m mantime -s shared-memory-id.
->station-manager.c = flags: -s shared-memory-id -o outfput_file(name for logfile).
->comptroller.c = flags: -d time -t stattimes -s shared-memory-id -o outfput_file(name for logfile).
``` 

Run commands
------------
Compile and execute:
```
make
make run
```
Clean:
```
make clean
```
