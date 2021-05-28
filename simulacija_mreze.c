#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>

#define BR_RACUNARA 10

/*
TODOs:

	- resiti problem sa rolloverom u vremenu
	- oba racunara moraju da prestanu transmisiju ako je Kolizija
	- cekanje od 50ms do 100ms pocinje od pocetka uspesne transmisije
	- mag.brojac nije zaredom uspesne, nego samo ukupno uspesnih prenosa unutar jedne sekunde

*/


pthread_mutex_t mutex;
sem_t bus_mutex;
time_t t;
struct timeval tv;
struct magistrala_t* magistrala;

int max_Col_wait_time = 0;
void* checker_thread(void* args);
void* pc_fun(void* args);

struct magistrala_t {
	int pt;			// početak transmisije u mikrosekundama
	int racunar_id; 	// id računara koji je počeo transmisiju
	int brojac; 	// brojač okvira prenetih bez prekida kroz mrežu
	int trenutni_broj;
};

int wait_time(int noofCol, int pc_id)
{
	int n = pow(2,noofCol);
	int t = 2 * (rand() % n);

	if(t > max_Col_wait_time) // gleda koliko je max exponencijalno vreme cekano bilo za sve racunare
		max_Col_wait_time = t;

	//	printf("pozvao racunar id %d - nofCol: %d - wait time: %d\n", pc_id, noofCol, ret_val);
	//printf("max cekanje za komp id %d je %d\n", pc_id, n);

	return t;
}

void* checker_thread(void* args)
{
	int total_time = 0;
	int ret_val = 0;
	int *res = malloc(sizeof(int));

	while(total_time < 60)
	{
		ret_val += magistrala -> trenutni_broj > magistrala -> brojac ?
		 			magistrala -> trenutni_broj : magistrala -> brojac;
		magistrala -> brojac = 0;
		magistrala -> trenutni_broj = 0;
		total_time++;
		printf("\t\tChecker thread proverio %d puta\n", total_time);
		sleep(1);
	}

	*res = ret_val;
	return (void*) res;
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
		// pokusaj da zauzmes upis u vreme magistrale
		//sem_wait(&bus_mutex);

		gettimeofday(&tv , NULL);
		int vreme = tv.tv_usec; // trenutno vreme u ms

		//printf("vremena: %d %d %d\n", vreme, curr_bus_time, magistrala -> racunar_id);
		if(magistrala -> racunar_id == 0)
		{
			pthread_mutex_lock(&mutex);
			// ako je magistrala prazna .. da li se u koliziji oba racunara prekidaju?
			// upisi podatke

			magistrala -> pt = vreme;
			magistrala -> racunar_id = id;
			nofCol = 0;
			magistrala -> trenutni_broj++;

			//printf("mag_brojac: %d, rac_id: %d\n", magistrala->brojac, magistrala->racunar_id);
			// zapocni transmisiju

			pthread_mutex_unlock(&mutex);
			usleep(10000);

			pthread_mutex_lock(&mutex);
			// oslobodi magistralu
			magistrala -> racunar_id = 0;

			pthread_mutex_unlock(&mutex);

			//sem_post(&bus_mutex);
			//printf("spava %d\n", sleep_time);
		}
		else if((magistrala->racunar_id != 0) && vreme - curr_bus_time >= 2000)
		{ // ako je magistrala zauzeta ali nije kolizija
			nofCol = 0;
			// sacekaj svoj red, 10ms
			//pthread_mutex_unlock(&mutex);
			usleep(10000);

			//sem_post(&bus_mutex);
			//	printf("cekanje sa %d i %d\n", vreme, curr_bus_time);
			//continue; // idi ispocetka while
		} else {
			// ako je kolizija
			if(nofCol < 10)
				nofCol++;
			else
				nofCol = 10;
			pthread_mutex_lock(&mutex);
			if(magistrala -> brojac < magistrala -> trenutni_broj)
				magistrala -> brojac = magistrala -> trenutni_broj;
			magistrala -> trenutni_broj = 0;
			// cekaj eksponencijalno po nofCol
			pthread_mutex_unlock(&mutex);
			int cekanje = wait_time(nofCol, id);
			//printf("Kolizija! Racunar %d staje i ceka %dms\n", id, cekanje);
			usleep(cekanje * 1000); // wait_time vraca vreme u ms
			//sem_post(&bus_mutex);
		}

		int sleep_time = (51 + (rand() % 100));
		usleep(1000 * sleep_time); // cekaj nasumicno 50ms - 150ms
	}
}

int main(){

	srand((unsigned) time(&t));

	magistrala = (struct magistrala_t*) malloc(sizeof(struct magistrala_t));
	magistrala -> racunar_id = 0;
	magistrala -> trenutni_broj = 0;

	pthread_mutex_init(&mutex, NULL);
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

	pthread_join(checker, (void**) &res);

	double iskoriscenost = (*res) / 6000.0;

//	printf("\nIskoriscenost mreze: %f\n", iskoriscenost);
	printf("Broj prenetih paketa bez kolizije: %d\nIskoriscenost mreze: %f\n", *res, iskoriscenost);

	printf("max cekanje ikad: %d", max_Col_wait_time);
	exit(0);
}
