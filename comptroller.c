#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>		/* For bool type */
#include <unistd.h>			/* For file descriptors */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include "structures.h"

bool running = true;
void signal_handler();

int main(int argc, char** argv){
  	/* For required arguments */
	if (argc == 9){
    	char *output_file, *buffer;
        int s, t, d, err, total_pas_arrive, total_pas_left, total_bus_in, 
        total_bus_out, position, time_remaining, p_in=0, p_out=0, tr=0, 
        vor_t, ask_t, pel_t, vor_b, ask_b, pel_b, vor_pa, ask_pa, pel_pa,
        vor_pl, ask_pl, pel_pl, bus_id, tt, lines, bus_type;
        void *shm;
        stman_comptr *st; counters *cn;
        FILE *read_fp;
        signal(SIGUSR2,signal_handler);

		/* For the order of line arguments */
		for (int jj=0 ; jj<argc ; jj++){
			if (!strcmp("-d", argv[jj])) d = atoi((char*)argv[jj+1]);
			if (!strcmp("-t", argv[jj])) t = atoi((char*)argv[jj+1]);
			if (!strcmp("-s", argv[jj])) s = atoi((char*)argv[jj+1]);
			if (!strcmp("-o", argv[jj])) output_file = (char*)argv[jj+1];
		}

		/* Attach the segment */
		shm = (void *) shmat(s, (void*)0, 0);
		if (*(int *) shm == -1) perror("Comptroller: Attachment.");

		st = &(*(stman_comptr *)(shm+sizeof(bus_stman)));
		cn = &(*(counters *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)));

		pid_t pid;
		if ((pid=fork())<0) printf("mystation.c: Error at fork() (exe station-manager)\n");
        else if(pid==0){
        	signal(SIGUSR2,signal_handler);
			while(running){
				total_pas_arrive=0; total_pas_left=0; total_bus_in=0; total_bus_out=0, time_remaining=0; lines=0,
				vor_t=0, ask_t=0, pel_t=0, vor_b=0, ask_b=0, pel_b=0, vor_pa=0, ask_pa=0, pel_pa=0,
        		vor_pl=0, ask_pl=0, pel_pl=0;
				/*	READ LOG FILE 	*/
				if (running){
					sleep(d);
					if(sem_wait(&(st->mutex)) < 0) perror ("Station-manager: wait(mutex).");
					read_fp = fopen(output_file, "a+");
					fscanf(read_fp, "%*[^\n]\n");
					while(fscanf(read_fp, "%d%8d%12d:%d:%d\t%7d:%d:%d\t%5d%12d%18d%18d\n", 
					&bus_id, &bus_type, &tt, &tt, &tt, &tt, &tt, &tt, &position, &p_in, &p_out, &tr) > 0){
						total_pas_left+=p_out;
						total_pas_arrive+=p_in;
						time_remaining+=tr;
						if(bus_type == 0){ vor_t+=tr; vor_b++; vor_pa+=p_in; vor_pl+=p_out; }
						else if(bus_type == 1){ ask_t+=tr; ask_b++; ask_pa+=p_in; ask_pl+=p_out; }
						else { pel_t+=tr; pel_b++; pel_pa+=p_in; pel_pl+=p_out; }
						lines++;
					}
					fclose(read_fp);
					if(sem_post(&(st->mutex)) < 0) perror ("Station-manager: post(mutex).");

					if(lines>0){
						printf("-----------------------------STATISTICS-----------------------------\n");
						printf("-> Average remaining time: %d\n", time_remaining/lines);
						if (vor_b > 0) printf("-> Average remaining time for VOR: %d\n", vor_t/vor_b);
						if (ask_b > 0) printf("-> Average remaining time for ASK: %d\n", ask_t/ask_b);
						if (pel_b > 0) printf("-> Average remaining time for PEL: %d\n", pel_t/pel_b);
						printf("-> Total passengers arrived: %d\n", total_pas_arrive);
						printf("-> Total passengers left: %d\n", total_pas_left);
						printf("-> Percent for passengers VOR - arrived: %d%% left: %d%%\n", vor_pa*100/total_pas_arrive, vor_pl*100/total_pas_left);
						printf("-> Percent for passengers ASK - arrived: %d%% left: %d%%\n", ask_pa*100/total_pas_arrive, ask_pl*100/total_pas_left);
						printf("-> Percent for passengers PEL - arrived: %d%% left: %d%%\n", pel_pa*100/total_pas_arrive, pel_pl*100/total_pas_left);
						printf("-> Total buses that have been served: %d\n", lines);
						printf("--------------------------------------------------------------------\n");
					}
				}
			}
			/* Dettach the segment */
			err = (int) shmdt((void *) shm);
			if (err == -1) perror ("Comptroller: Detachment.");
			return 0;
		}
		while(running){
			if(running){
					sleep(t);

					if(sem_wait(&(st->ledger)) < 0) perror ("Comptroller: wait(ledger).");
					printf("---------------------------STATION-STATUS---------------------------\n");
					printf("-> Number of buses in station: %d\n", cn->buses);
					printf("-> Free positions of VOR: %d/%d\n", cn->free_vor, cn->pos_vor);
					printf("-> Free positions of ASK: %d/%d\n", cn->free_ask, cn->pos_ask);
					printf("-> Free positions of PEL: %d/%d\n", cn->free_pel, cn->pos_pel);
					printf("-> Passengers that disembark from current buses: %d\n", cn->passengers_out);
					printf("--------------------------------------------------------------------\n");
					if(sem_post(&(st->ledger)) < 0) perror ("Comptroller: post(ledger).");
			}
		}
		kill(pid,SIGUSR2);
		wait(NULL);

		/* Dettach the segment */
		err = (int) shmdt((void *) shm);
		if (err == -1) perror ("Comptroller: Detachment.");

		printf("Comptroller terminates.\n");
	}
}

void signal_handler(int sig_num){
	if (sig_num == SIGUSR2) running = false;
	return;
}