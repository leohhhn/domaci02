#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>

#define BR_RACUNARA 10

sem_t bus_mutex;
time_t t;
struct timeval tv;
struct magistrala_t* magistrala;

void* checker_thread(void* args);
void* pc_fun(void* args);

struct magistrala_t {
	int pt;			// početak transmisije u mikrosekundama
	int racunar_id; 	// id računara koji je počeo transmisiju
	int brojac; 	// brojač okvira prenetih bez prekida kroz mrežu
};

int wait_time(int noofCol)
{
	int base = 2;
    int result = 1;
    for (;;)
    {
        if (noofCol & 1)
            result *= base;
        noofCol >>= 1;
        if (!noofCol)
            break;
        base *= base;
    }
    return 2 * (rand() % result);
}

void* checker_thread(void* args)
{
	int total_time = 0;
	int* ret_val = malloc(4);

	while(total_time < 2)
	{
	ret_val += magistrala -> brojac;
	total_time++;
	printf("Checker thread proverio %d puta\n", total_time);
	sleep(1);
	}

	pthread_exit((void*) ret_val);

}

void* pc_fun(void* args)
{
// funkcija za svaki thread/racunar
// racunari ce pokusavati da posalju okvire dok im thread_checker ne kaze da su poslali sve sto treba

	int id = *((int *) args);
	int curr_bus_time;
	int nofCol = 0; // br uzastopnih kolizija

	while(1)
	{

		curr_bus_time = magistrala -> pt;
		pthread_t transmission_thread;

		// pokusaj da zauzmes upis u vreme magistrale
		sem_wait(&bus_mutex);

		gettimeofday(&tv , NULL);
		int vreme; // trenutno vreme u ms

		if(magistrala -> racunar_id == 0) // ako je magistrala prazna .. da li se u koliziji oba racunara prekidaju?
		{
			// upisi podatke
			vreme = tv.tv_usec;
			magistrala -> pt = vreme;
			magistrala -> racunar_id = id;
			nofCol = 0;
			magistrala -> brojac++;
		//	printf("mag_brojac: %d\n rac_id: %d\n", magistrala->brojac, magistrala->racunar_id);
			// zapocni transmisiju
			usleep(10000);
			// oslobodi magistralu
			magistrala -> racunar_id = 0;
			sem_post(&bus_mutex);
		}
		else if((magistrala->racunar_id != 0) && (vreme - curr_bus_time >= 2000))
		{ // ako je magistrala zauzeta ali nije kolizija
			nofCol = 0;
			// sacekaj svoj red, 10ms
			usleep(10000);
			sem_post(&bus_mutex);
			continue; // idi ispocetka while
		} else {
			// ako je kolizija
			if(nofCol < 10)
				nofCol++;
			else
				nofCol = 10;
			// cekaj eksponencijalno po nofCol
			sem_post(&bus_mutex);
			usleep(wait_time(nofCol));
		}

		usleep(1000 * (51 + rand() % 100)); // cekaj nasumicno 50ms - 150ms
	}
}



int main(){

	srand((unsigned) time(&t));

	magistrala = (struct magistrala_t*) malloc(sizeof(struct magistrala_t));
	magistrala -> racunar_id = 0;

	sem_init(&bus_mutex, 0, 1);
	pthread_t threads[BR_RACUNARA];
	pthread_t checker;
	int ids[BR_RACUNARA];

	for(int i = 0; i < 10; i++) {
		ids[i] = i + 1;
		pthread_create(&threads[i], NULL, pc_fun, (void*)(&ids[i]));
	}

	pthread_create(&checker, NULL, checker_thread, NULL);
	int* res;

	pthread_join(checker, (void*) res);
	res = (int*) res;
	printf("res: %d", *res);
//	printf("Broj prenetih paketa bez kolizije: %d\nIskoriscenost mreze: %d\n", /6000);

	exit(0);
}
