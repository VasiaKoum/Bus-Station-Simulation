#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include "operations.h"
#include "structures.h"

enum type type_enum(char *str){
	if (strcmp(str, "ASK") == 0) return ASK;
	else if (strcmp(str, "PEL") == 0) return PEL;
	else return VOR;
}

void shm_init(void *shm, int total_buses, int positions[3][2]){
	bus_stman *bs = &(*(bus_stman *)shm);
	stman_comptr *st = &(*(stman_comptr *)(shm+sizeof(bus_stman)));
	counters *cn = &(*(counters *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)));
	reference_ledger *rl = &(*(reference_ledger *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)+sizeof(counters)));

	if (sem_init(&(bs->request_in),1 ,1) != 0) perror("request_in: Couldn't initialize.");
	if (sem_init(&(bs->request_out),1 ,1) != 0) perror("request_out: Couldn't initialize.");
	if (sem_init(&(bs->bus_in),1 ,0) != 0) perror("bus_in: Couldn't initialize.");
	if (sem_init(&(bs->bus_out),1 ,0) != 0) perror("bus_out: Couldn't initialize.");
	if (sem_init(&(bs->ready),1 ,0) != 0) perror("ready: Couldn't initialize.");
	if (sem_init(&(bs->change_reqs),1 ,1) != 0) perror("change_reqs: Couldn't initialize.");
	if (sem_init(&(bs->man),1 ,1) != 0) perror("man: Couldn't initialize.");
	bs->requests = 0;
	bs->waiting = false;
	bs->buses = 0;

	if (sem_init(&(st->mutex),1 ,1) != 0) perror("mutex: Couldn't initialize.");
	if (sem_init(&(st->ledger),1 ,1) != 0) perror("ledger: Couldn't initialize.");

	cn->buses=0;
	cn->free_vor=positions[0][1];
	cn->free_ask=positions[1][1];
	cn->free_pel=positions[2][1];
	cn->pos_vor=positions[0][1];
	cn->pos_ask=positions[1][1];
	cn->pos_pel=positions[2][1];
	cn->passengers_out=0;

	for (int i=0; i<(positions[0][1]+positions[1][1]+positions[2][1]);i++) rl[i].bus_status = false;
}

int check_position(reference_ledger *rl, stman_comptr *st, counters *cn, int vor_min, int vor_max, int ask_min, int ask_max, int pel_min, int pel_max, enum type btype){
	int min, max;
	if(btype == VOR) { min = vor_min; max = pel_max; }
	if(btype == ASK) { min = ask_min; max = pel_max; }
	if(btype == PEL) { min = pel_min; max = pel_max; }

	for (int i=min; i<=max ;i++){
		if (!rl[i].bus_status){
			
			if(sem_wait(&(st->ledger)) < 0) perror ("Operations: wait(ledger).");
			if(i<=vor_max) cn->free_vor--;
			else if(i<=ask_max) cn->free_ask--;
			else cn->free_pel--;
			cn->buses++;
			if(sem_post(&(st->ledger)) < 0) perror ("Operations: post(ledger).");

			return i;
		}
	}
	return -1;
}

void reduce_pos(stman_comptr *st, counters *cn, int vor_max, int ask_max, int pel_max, int i){
	if(sem_wait(&(st->ledger)) < 0) perror ("Operations: wait(ledger).");
	if(i<=vor_max) cn->free_vor++;
	else if(i<=ask_max) cn->free_ask++;
	else cn->free_pel++;
	cn->buses--;
	if(sem_post(&(st->ledger)) < 0) perror ("Operations: post(ledger).");
}

void shm_destroy(void *shm){
	bus_stman *bs = &(*(bus_stman *)shm);
	stman_comptr *st = &(*(stman_comptr *)(shm+sizeof(bus_stman)));
	counters *cn = &(*(counters *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)));
	reference_ledger *rl = &(*(reference_ledger *)(shm+sizeof(bus_stman)+sizeof(stman_comptr)+sizeof(counters)));

	if (sem_destroy(&(bs->request_in)) != 0) perror("request_in: Couldn't destroy.");
	if (sem_destroy(&(bs->request_out)) != 0) perror("request_out: Couldn't destroy.");
	if (sem_destroy(&(bs->bus_in)) != 0) perror("bus_in: Couldn't destroy.");
	if (sem_destroy(&(bs->bus_out)) != 0) perror("bus_out: Couldn't destroy.");
	if (sem_destroy(&(bs->ready)) != 0) perror("ready: Couldn't destroy.");
	if (sem_destroy(&(bs->change_reqs)) != 0) perror("change_reqs: Couldn't destroy.");
	if (sem_destroy(&(bs->man)) != 0) perror("man: Couldn't destroy.");

	if (sem_destroy(&(st->mutex)) != 0) perror("mutex: Couldn't destroy.");
	if (sem_destroy(&(st->ledger)) != 0) perror("ledger: Couldn't destroy.");
}

