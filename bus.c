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
#include <time.h>
#include <fcntl.h>
#include "operations.h"
#include "structures.h"

int main(int argc, char** argv){
  	/* For required arguments */
	if (argc == 13){
    	char *t;
        int s, err, position, n, p, c, min, max, m;
		void *shm;
		bus_stman *bs; stman_comptr *st; counters *cn; reference_ledger *rl;
        srand(time(0));

		/* For the order of line arguments */
		for (int jj=0 ; jj<argc ; jj++){
			if (!strcmp("-t", argv[jj])) t = (char*)argv[jj+1];
			if (!strcmp("-n", argv[jj])) n = atoi((char*)argv[jj+1]);
			if (!strcmp("-c", argv[jj])) c = atoi((char*)argv[jj+1]);
			if (!strcmp("-p", argv[jj])) p = atoi((char*)argv[jj+1]);
			if (!strcmp("-m", argv[jj])) m = atoi((char*)argv[jj+1]);
			if (!strcmp("-s", argv[jj])) s = atoi((char*)argv[jj+1]);
		}

		/* Attach the segment */
		shm = (void *) shmat(s, (void*)0, 0);
		if (*(int *) shm == -1) perror("Bus: Attachment.");

		bs = &(*(bus_stman *)shm);
		st = &(*(stman_comptr *)(shm+sizeof(bus_stman)));
		cn = &(*(counters *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)));
		rl = &(*(reference_ledger *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)+sizeof(counters)));
		printf("Bus arrived -> [%d] type: [%s] \n", getpid(), t);
		/* Increase requests */
		if(sem_wait(&(bs->change_reqs)) < 0) perror("Bus: wait(change_reqs)_1.");
		bs->requests++;
		if(sem_post(&(bs->change_reqs)) < 0) perror("Bus: post(change_reqs)_1.");

		/* Make a request to inform station-manager */
		if(sem_wait(&(bs->request_in)) < 0) perror("Bus: wait(request_in).");
		/* Inform station-manager that is request to enter and change bus_type */
		bs->flag_in = true;
		bs->bus_type = type_enum(t);

		/* Wake up station-manager */
		if(sem_post(&(bs->ready)) < 0) perror("Bus: post(ready)_1.");
		/* Wait for station-manager to let us enter */
		if(sem_wait(&(bs->bus_in)) < 0) perror("Bus: wait(bus_in).");
		/* Unlock the request */
		if(sem_post(&(bs->request_in)) < 0) perror("Bus: post(request_in).");

		/* Arrive to the position and disembark the passengers */
		position = bs->position_in;
		printf("The station-manager give -> [%d] type: [%s] the position [%d]\n", getpid(), t, position);

		/* Drive for position - one bus in & out ONLY */
		if(sem_wait(&(bs->man)) < 0) perror("Bus: wait(man).");
		sleep(m);
		if(sem_post(&(bs->man)) < 0) perror("Bus: post(man).");	

		if(sem_wait(&(st->ledger)) < 0) perror ("Bus: wait(ledger).");
		cn->passengers_out+=n;
		if(sem_post(&(st->ledger)) < 0) perror ("Bus: post(ledger).");

		/* Decrease requests */
		if(sem_wait(&(bs->change_reqs)) < 0) perror("Bus: wait(change_reqs)_2.");
		bs->requests--;
		if(sem_post(&(bs->change_reqs)) < 0) perror("Bus: post(change_reqs)_2.");				

		if(sem_wait(&(bs->change_reqs)) < 0) perror("Bus: wait(change_reqs)_2.");
		bs->buses++;
		if(sem_post(&(bs->change_reqs)) < 0) perror("Bus: post(change_reqs)_2.");

		/* Update the reference-ledger */
		rl[position].bus_type = type_enum(t);
		rl[position].licence_plate = getpid();
		rl[position].passengers_out = n;

		/* Waiting for new passengers */
		sleep(p);
		rl[position].passengers_in = (rand()%c)+1;
		
		/* Increase requests */
		if(sem_wait(&(bs->change_reqs)) < 0) perror("Bus: wait(change_reqs)_3.");
		bs->requests++;
		if(sem_post(&(bs->change_reqs)) < 0) perror("Bus: post(change_reqs)_3.");

		/* Make a request to inform station-manager */
		if(sem_wait(&((*(bus_stman *)shm).request_out)) < 0) perror("Bus: wait(request_out).");
		/* Inform station-manager that is request to leave */
		bs->flag_out = true;
		bs->position_out = position;

		/* Wake up station-manager */
		if(sem_post(&((*(bus_stman *)shm).ready)) < 0) perror("Bus: post(ready)_2.");	
		/* Wait for station-manager to let us leave */
		if(sem_wait(&((*(bus_stman *)shm).bus_out)) < 0) perror("Bus: wait(bus_out).");

		/* Decrease requests */
		if(sem_wait(&(bs->change_reqs)) < 0) perror("Bus: wait(change_reqs)_4.");
		bs->requests--;
		bs->buses--;
		if (bs->buses==0) if(sem_post(&(bs->ready)) < 0) perror("Bus: post(ready)_1.");
		if(sem_post(&(bs->change_reqs)) < 0) perror("Bus: post(change_reqs)_4.");	

		/* Update reference ledger about bus_status */
		rl[position].bus_status = false;

		if(sem_wait(&(st->ledger)) < 0) perror ("Bus: wait(ledger).");
		cn->passengers_out-=n;
		if(sem_post(&(st->ledger)) < 0) perror ("Bus: post(ledger).");

		printf("Bus leave -> [%d] type: [%s] from position [%d]\n", rl[position].licence_plate, t, position);
		/* Unlock the request */
		if(sem_post(&((*(bus_stman *)shm).request_out)) < 0) perror("Bus: post(request_out).");
		//printf("Bus departs... with %d buses in station and %d requests.\n", bs->buses, bs->requests);

		/* Drive for position - one bus in & out ONLY */
		if(sem_wait(&(bs->man)) < 0) perror("Bus: wait(man).");
		sleep(m);
		if(sem_post(&(bs->man)) < 0) perror("Bus: post(man).");	

		/* Dettach the segment */
		err = (int) shmdt((void *) shm);
		if (err == -1) perror("Bus: Detachment.");
	}
}