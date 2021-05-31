#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>

#define BR_RACUNARA 10

pthread_mutex_t mutex;
pthread_mutex_t global;
time_t t;
struct timeval tv;
struct magistrala_t* magistrala;
void* checker_thread(void* args);
void* pc_fun(void* args);

int collision_happened = 0;
int pc_id_coll = 0;
int test_mode = 0;
int max_Col_wait_time = 0;

struct magistrala_t {
	int pt;			// početak transmisije u mikrosekundama
	int racunar_id; 	// id računara koji je počeo transmisiju
	int brojac; 	// brojač okvira prenetih bez prekida kroz mrežu
};

int wait_time(int noofCol, int pc_id)
{
	int n = pow(2, noofCol);
	int t = 2 * (rand() % n);

	if(t > max_Col_wait_time) // gleda koliko je max exponencijalno vreme cekano bilo za sve racunare
		max_Col_wait_time = t;

	return t;
}

void* checker_thread(void* args)
{
	int total_time = 0; // 60 sekundi
	int ret_val = 0;
	int *res = malloc(sizeof(int));
	int run_time = 60;

	if(test_mode == 0)
		run_time = 60;
	else
		run_time = 5;

	while(total_time < run_time)
	{
		pthread_mutex_lock(&mutex);
		ret_val += magistrala -> brojac;
		magistrala -> brojac = 0;
		pthread_mutex_unlock(&mutex);
		printf("Time elapsed: %ds\n", total_time + 1);
		total_time++;
		sleep(1);
	}

	*res = ret_val;
	return (void*) res;
}

void* pc_fun(void* args)
{
	// funkcija za svaki thread/racunar

	int id = *((int *) args);
	int curr_bus_time = 0; // vreme procitano sa magistrale
	int nofCol = 0; // br uzastopnih kolizija

	while(1)
	{
		int izasao = 0;
		gettimeofday(&tv , NULL);

		long int vreme_usec = tv.tv_usec; // trenutno vreme u mikrosekundama
		long int vreme_sec = tv.tv_sec; // trenutno vreme u sekundama
		long int real_time_usec = vreme_sec * 1000000 + vreme_usec;
		curr_bus_time = magistrala -> pt;

		if(magistrala -> racunar_id == 0)
		{ // ako je magistrala slobodna

			pthread_mutex_lock(&mutex);
			// upisi podatke u magistralu
			magistrala -> pt = real_time_usec;
			magistrala -> racunar_id = id;
			pthread_mutex_unlock(&mutex);
			nofCol = 0;

			// 	zapocni transmisiju
			for(int i = 0; i < 100; i++)
			{
				// granulisi sleep da bi proveravao da li si upao u imposed koliziju
				if(collision_happened && pc_id_coll == id)
				{
					izasao = 1;
					break;
				}
				usleep(100);
			}

			if(izasao)	{
				pthread_mutex_lock(&global);
				magistrala -> racunar_id = 0; // skidaj se sa magistrale jer si u koliziji
				collision_happened = 0; // resetuj da li se kolizija desila
				pc_id_coll = 0; // resetuj ID
				pthread_mutex_unlock(&global);
				izasao = 0;

				if(nofCol < 10)
					nofCol++;
				else
					nofCol = 10;

				// cekaj eksponencijalno po nofCol
				int cekanje = wait_time(nofCol, id);
			//	printf("Imposed Kolizija! Racunar %d staje i ceka %dms\n", id, cekanje);
				usleep(cekanje * 1000); // wait_time vraca vreme u ms
				continue;
			}

			pthread_mutex_lock(&mutex);
			magistrala -> brojac++;
			magistrala -> racunar_id = 0; // oslobodi magistralu
			pthread_mutex_unlock(&mutex);

			int sleep_time = 1000 * (51 + (rand() % 100));
			usleep(sleep_time); // cekaj nasumicno 50ms - 150ms;
		}
		else if((magistrala->racunar_id != 0) && (abs(real_time_usec - curr_bus_time) >= 2000))
		{
			// ako je magistrala zauzeta ali nije kolizija
			nofCol = 0;
			// sacekaj svoj red, 10ms
			usleep(10000);
		} else {
			// ako je cista kolizija
			if(nofCol < 10)
				nofCol++;
			else
				nofCol = 10;

			pthread_mutex_lock(&global);
			pc_id_coll = magistrala -> racunar_id;
			collision_happened = 1;
			pthread_mutex_unlock(&global);

			// cekaj eksponencijalno po nofCol
			int cekanje = wait_time(nofCol, id);
		//	printf("Kolizija sa racunarom %d! Racunar %d staje i ceka %dms\n", pc_id_coll, id, cekanje);
			usleep(cekanje * 1000); // wait_time vraca vreme u ms
		}
	}
}

int main(){

	srand((unsigned) time(&t));

	magistrala = (struct magistrala_t*) malloc(sizeof(struct magistrala_t));
	magistrala -> racunar_id = 0;

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&global, NULL);
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

	double iskoriscenost = 0;
	if(test_mode == 0)
	 	iskoriscenost = (*res) / 6000.0;
	else
		iskoriscenost = (*res) / 500.0;

	printf("Broj prenetih paketa kroz mrezu: %d\nIskoriscenost mreze: %d%\n", *res, (int) (iskoriscenost * 100));
	printf("Najduze cekanje: %d\n", max_Col_wait_time);
	exit(0);
}
