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
*/

int collision_happened = 0;
int pc_id_coll = 0;

pthread_mutex_t mutex;
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

	while(total_time < 5)
	{
		pthread_mutex_lock(&mutex);
		ret_val += magistrala -> brojac;
		magistrala -> brojac = 0;
		pthread_mutex_unlock(&mutex);

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
	int curr_bus_time = 0;
	int nofCol = 0; // br uzastopnih kolizija

	while(1)
	{
		curr_bus_time = magistrala -> pt;
		// pokusaj da zauzmes upis u vreme magistrale
		//sem_wait(&bus_mutex);

		gettimeofday(&tv , NULL);
		int vreme = tv.tv_usec; // trenutno vreme u ms
		int izasao = 0;

		//printf("vremena: %d %d %d\n", vreme, curr_bus_time, magistrala -> racunar_id);
		if(magistrala -> racunar_id == 0)
		{
			int izasao = 0;
			pthread_mutex_lock(&mutex);
			// upisi podatke
			magistrala -> pt = vreme;
			magistrala -> racunar_id = id;
			nofCol = 0;
			pthread_mutex_unlock(&mutex);

			// zapocni transmisiju
			for(int i = 0; i < 1000; i++)
			{
				// granulisi sleep da proveravas da li si upao u koliziju
				if(collision_happened && pc_id_coll == id)
				{
					izasao = 1;
					break;
				}
				usleep(10);
			}

			if(izasao)	{
				pthread_mutex_lock(&mutex);
				magistrala -> racunar_id = 0; // skidaj se sa magistrale jer si u koliziji
				collision_happened = 0; // resetuj da li se kolizija desila
				pc_id_coll = 0; // resetuj ID
				pthread_mutex_unlock(&mutex);
				izasao = 0;

				if(nofCol < 10)
					nofCol++;
				else
					nofCol = 10;

				// cekaj eksponencijalno po nofCol
				int cekanje = wait_time(nofCol, id);
				printf("Imposed Kolizija! Racunar %d staje i ceka %dms\n", id, cekanje);
				usleep(cekanje * 1000); // wait_time vraca vreme u ms

			}

			int sleep_time = 1000 * (51 + (rand() % 100));
			usleep(sleep_time); // cekaj nasumicno 50ms - 150ms

			pthread_mutex_lock(&mutex);
			magistrala -> brojac++;
			magistrala -> racunar_id = 0; // oslobodi magistralu
			pthread_mutex_unlock(&mutex);


			
			//sem_post(&bus_mutex);
			//printf("spava %d\n", sleep_time);
		}
		else if((magistrala->racunar_id != 0) && vreme - curr_bus_time >= 2000) // nije dobar uslov za 2k
		{ // ako je magistrala zauzeta ali nije kolizija
			nofCol = 0;
			// sacekaj svoj red, 10ms
			usleep(10000);
			//printf("komp %d ceka, razlika: %d\n", id, vreme - curr_bus_time);
		} else {
			// ako je cista kolizija
			if(nofCol < 10)
				nofCol++;
			else
				nofCol = 10;

			pthread_mutex_lock(&mutex);
			collision_happened = 1;
			pc_id_coll = magistrala -> racunar_id;
			pthread_mutex_unlock(&mutex);

			// cekaj eksponencijalno po nofCol
			int cekanje = wait_time(nofCol, id);
			printf("Kolizija sa racunarom %d! Racunar %d staje i ceka %dms\n", pc_id_coll, id, cekanje);
			usleep(cekanje * 1000); // wait_time vraca vreme u ms
			//sem_post(&bus_mutex);
		}
	}
}

int main(){

	srand((unsigned) time(&t));

	magistrala = (struct magistrala_t*) malloc(sizeof(struct magistrala_t));
	magistrala -> racunar_id = 0;

	pthread_mutex_init(&mutex, NULL);
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
	double iskoriscenost = (*res) / 500.0;

	//printf("\nIskoriscenost mreze: %f\n", iskoriscenost);
	printf("Broj prenetih paketa kroz mrezu: %d\nIskoriscenost mreze: %d%\n", *res, (int) round(iskoriscenost * 100));

	//printf("max cekanje ikad: %d", max_Col_wait_time);
	exit(0);
}
