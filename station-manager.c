#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>		/* For bool type */
#include <unistd.h>			/* For file descriptors */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include "operations.h"
#include "structures.h"

int main(int argc, char** argv){
  	/* For required arguments */
	if (argc == 7){
        char *output_file;
        int s, check_pos, i, pid_comp;
        bool exiting = true;
        void *shm;
        struct tm arrival, depart;
        time_t dtime, wtime;
        FILE *write_fp;
        bus_stman *bs; stman_comptr *st; counters *cn; reference_ledger *rl;

        for (int jj=0 ; jj<argc ; jj++){
        	if (!strcmp("-s", argv[jj])) s=atoi(argv[jj+1]);
        	if (!strcmp("-o", argv[jj])) output_file=argv[jj+1];
        	if (!strcmp("-p", argv[jj])) pid_comp=atoi(argv[jj+1]);
        }

        /* Attach the segment */
		shm = (void *) shmat(s, (void*)0, 0);
		if (*(int *) shm == -1) perror("Station-manager: Attachment.");

		bs = &(*(bus_stman *)shm);
		st = &(*(stman_comptr *)(shm+sizeof(bus_stman)));
		cn = &(*(counters *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)));
		rl = &(*(reference_ledger *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)+sizeof(counters)));

		while(exiting){
			i=0;
			while((bs->requests==0) && (bs->buses==0) && i<5){
				if((bs->requests==0) && (bs->buses==0) && (i==4)) exiting = false;
				else{
					printf("Waiting for requests...\n");
					sleep(3);
				}
				i++;
			}
			if(exiting){
				if(!bs->waiting){
					if(sem_wait(&(bs->ready)) < 0) perror ("Station-manager: wait(ready)_1.");
				}
				/* Receive a request for bus to enter */
				if(bs->flag_in){
					if((check_pos = check_position(rl, st, cn, 0, cn->pos_vor-1, cn->pos_vor, cn->pos_ask+cn->pos_vor-1, cn->pos_ask+cn->pos_vor, cn->pos_ask+cn->pos_vor+cn->pos_pel-1, bs->bus_type))>-1){
						bs->position_in = check_pos;
						bs->flag_in = false;
						bs->waiting = false;
						rl[check_pos].bus_status = true;
						rl[check_pos].time = time(NULL);
						if(sem_post(&(bs->bus_in)) < 0) perror ("Station-manager: post(bus_in).");
					}
					else { bs->waiting = true;}
				}
				if((bs->waiting) && (bs->buses>0)) if(sem_wait(&(bs->ready)) < 0) perror ("Station-manager: wait(ready)_2.");
				if(bs->flag_out){
					bs->flag_out = false;
					arrival = *localtime(&rl[bs->position_out].time);
					dtime = time(NULL);
					depart = *localtime(&dtime);
					wtime = dtime - rl[bs->position_out].time;
					reduce_pos(st, cn, cn->pos_vor-1, cn->pos_ask+cn->pos_vor-1, cn->pos_ask+cn->pos_vor+cn->pos_pel-1, bs->position_out);

					/*	WRITE LOG FILE 	*/
					if(sem_wait(&(st->mutex)) < 0) perror ("Station-manager: wait(mutex).");
					write_fp = fopen(output_file, "a+");
					fprintf(write_fp, "%d%8d%12d:%d:%d\t%7d:%d:%d\t%5d%12d%18d%18d\n", 
						rl[bs->position_out].licence_plate, rl[bs->position_out].bus_type, arrival.tm_hour, arrival.tm_min, arrival.tm_sec, 
						depart.tm_hour, depart.tm_min, depart.tm_sec, bs->position_out, rl[bs->position_out].passengers_in, rl[bs->position_out].passengers_out,(int)wtime);
					fclose(write_fp);
					if(sem_post(&(st->mutex)) < 0) perror ("Station-manager: post(mutex).");

					if(sem_post(&(bs->bus_out)) < 0) perror ("Station-manager: post(bus_out).");
				}
			}
		}

		/* Dettach the segment */
		if(((int) shmdt((void *) shm)) < 0) perror ("Station-manager: Detachment.");
		printf("Station-manager terminates.\n");

		kill(pid_comp,SIGUSR2);
	}
}