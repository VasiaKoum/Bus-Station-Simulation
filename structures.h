#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

enum type{VOR, ASK, PEL};

typedef struct{
	bool flag_in;
	bool flag_out;
	bool waiting;
	enum type bus_type;
	int position_in;
	int position_out;
	int requests;
	int buses;

	sem_t request_in;
	sem_t request_out;
	sem_t bus_in;
	sem_t bus_out;
	sem_t ready;
	sem_t change_reqs;
	sem_t man;
} bus_stman;

typedef struct{
	sem_t mutex;
	sem_t ledger;
} stman_comptr;

typedef struct{
	int buses;
	int free_ask, free_vor, free_pel;
	int pos_ask, pos_vor, pos_pel;
	int passengers_out;
} counters;

typedef struct{
	enum type bus_type;
	time_t time;
	bool bus_status;
	int licence_plate;
	int passengers_in;
	int passengers_out;
} reference_ledger;
#endif