all: simulacija_mreze.c
	gcc simulacija_mreze.c -o sim_mreze -pthread -lm

clean:
	rm sim_mreze
