#include "structures.h"
void shm_init(void*, int, int positions[3][2]);
void shm_destroy(void*);
enum type type_enum(char*);
int check_position(reference_ledger*, stman_comptr*, counters*, int, int, int, int, int, int, enum type);
void reduce_pos(stman_comptr*, counters*, int, int, int, int);