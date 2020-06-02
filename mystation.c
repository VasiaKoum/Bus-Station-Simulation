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
#include <semaphore.h>
#include "operations.h"
#include "structures.h"

int main(int argc, char** argv){
  	/* For required arguments */
	if (argc == 7){
    	char *l, *e, *c;
		/* For the order of line arguments */
		for (int jj=0 ; jj<argc ; jj++){
			if (!strcmp("-l", argv[jj])) l = (char*)argv[jj+1];
			if (!strcmp("-e", argv[jj])) e = (char*)argv[jj+1];
			if (!strcmp("-c", argv[jj])) c = (char*)argv[jj+1];
		}
		FILE *read_fp, *write_fp;
		char type[3][4], time[5], stattimes[5], shmid[15], sem1[15], sem2[15], output_file[40], pid_comp[15];
		int positions[3][2], total_buses, total_positions, total_size, id=0, err=0;
		void *shm;
		sem_t *semaph;
		
		read_fp = fopen(l, "rb");
		if (read_fp == NULL) exit(0);
		fscanf(read_fp, "%s", output_file);
		fscanf(read_fp, "%s %s", time, stattimes);
		fscanf(read_fp,"%s %d %d",type[0], &positions[0][0], &positions[0][1]);
		fscanf(read_fp,"%s %d %d",type[1], &positions[1][0], &positions[1][1]);
		fscanf(read_fp,"%s %d %d",type[2], &positions[2][0], &positions[2][1]);

		total_positions = positions[0][1] + positions[1][1] + positions[2][1];
		total_buses = positions[0][0] + positions[1][0] + positions[2][0];
		total_size = sizeof(bus_stman)+sizeof(stman_comptr)+sizeof(counters)+total_positions*sizeof(reference_ledger);

		/* Initiallize the log file */
		/* BUS_ID BUS_TYPE TIME_ARRIVAL TIME_DEPART POSITION PASSENGERS_LEFT PASSENGERS_TAKE TIME_WAIT */
		write_fp = fopen(output_file, "w+");
		fprintf(write_fp, "BUS_ID - BUS_TYPE - TIME_ARRIVAL - TIME_DEPART - POSITION - PASSENGERS_LEFT - PASSENGERS_TAKE - TIME_WAIT\n");
		fclose(write_fp);
		

		/* Make shared memory segment */
		id = shmget(IPC_PRIVATE,total_size,0666); 
		if (id == -1) perror ("Creation");
		else printf("Allocated Shared Memory with ID: %d\n",(int)id);
		sprintf(shmid, "%d", id);

		/* Attach the segment */
		shm = (void *) shmat(id, (void*)0, 0);
		if (*(int *) shm == -1) perror("Attachment.");
		else printf("Just Attached Shared Memory whose content is: ...\n");

		/* Initialize shared segment*/
		shm_init(shm, total_buses, positions);

		pid_t pid;
		/* EXE comptroller */
		if ((pid=fork())<0) printf("mystation.c: Error at fork() (exe comptroller)\n");
        else if(pid==0){
            fclose(read_fp);
            char *args[]={"./comptroller", "-d", time, "-t", stattimes, "-s", shmid, "-o", output_file, NULL};
            execv(args[0],args);
		}
		sprintf(pid_comp, "%d", pid);

		/* EXE station-manager */
		if ((pid=fork())<0) printf("mystation.c: Error at fork() (exe station-manager)\n");
        else if(pid==0){
        	fclose(read_fp);
       		char *args[]={"./station-manager", "-s", shmid, "-o", output_file, "-p", pid_comp, NULL};
            execv(args[0],args);
		}

		if (strcmp(e, "YES")==0){
			char type_bus[4], passengers[5], capacity[5], parkperiod[5], mantime[5];
			/* EXE bus */
			while(fscanf(read_fp,"%s %s %s %s %s",type_bus, passengers, capacity, parkperiod, mantime)>=1){
            	if ((pid=fork())<0) printf("mystation.c: Error at fork() (exe bus)\n");
            	else if(pid==0){
            		fclose(read_fp);
            		char *args[]={"./bus", "-t", type_bus, "-n", passengers, "-c", capacity, "-p", parkperiod, "-m", mantime, "-s", shmid, NULL};
            		execv(args[0],args);
				}
			}
		}
		else if (strcmp(e, "NO")==0){
			printf("Waiting for you to execute the bus executable.\n");
		}
		fclose(read_fp);
		while(wait(NULL)>0);

		/* Remove segment */
		err = shmctl(id, IPC_RMID, 0);
		if (err == -1) perror ("Removal.");
		else printf("Just Removed Shared Segment. %d\n", (int)(err));

		shm_destroy(shm);
		if(strcmp(c, "YES")==0){
			if (remove(output_file) == 0) printf("Output file deleted successfully\n"); 
   			else printf("Unable to delete the output file.\n");
		}
		printf("Mystation terminates.\n");
	}
}