#include "queue.c"
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 30; // frequency of emergency gift requests from New Zealand

void* ElfA(void *arg); // the one that can paint
void* ElfB(void *arg); // the one that can assemble
void* Santa(void *arg); 
void* ControlThread(void *arg); // handles printing and queues (up to you)
void init();
void create_threads();
void kill_threads();
void destroy_mutex();
void print_all_queues();

time_t start,end;

pthread_t tidA;
pthread_t tidB;
pthread_t tidS;
pthread_t tidC;

pthread_mutex_t packaging_mutex;
pthread_mutex_t painting_mutex;
pthread_mutex_t assembly_mutex;
pthread_mutex_t qa_mutex;
pthread_mutex_t delivery_mutex;
pthread_mutex_t qa_task_array_mutex;

Queue *packaging_q;  // 0
Queue *painting_q;   // 1
Queue *assembly_q;   // 2
Queue *qa_q;         // 3
Queue *delivery_q;   // 4

bool qa_tasks[1000];
int qa_task_id;

int gift_id;

int t;

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock(&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}

double check_time(time_t start, time_t end) {
	pthread_sleep(t);
	time(&end);
	//printf ("We are at %.2lf seconds.\n", difftime(end,start));
	return difftime(end,start);
}

void create_request() {
    int random_type = rand() % 20;
    Task t;
    t.ID = gift_id++;
    printf("gift id: %d  ---  ",gift_id-1);
    if (random_type < 8) {
    	printf("type 1\n");
    	t.type = 1;
    	pthread_mutex_lock(&packaging_mutex);
    	Enqueue(packaging_q,t);
    	pthread_mutex_unlock(&packaging_mutex);
    } else if (random_type < 12) {
    	printf("type 2\n");
    	t.type = 2;
    	t.qa_task_id = -1;
    	pthread_mutex_lock(&painting_mutex);
    	Enqueue(painting_q,t);
    	pthread_mutex_unlock(&painting_mutex);
    } else if (random_type < 16) {
    	printf("type 3\n");
    	t.type = 3;
    	t.qa_task_id = -1;
    	pthread_mutex_lock(&assembly_mutex);
    	Enqueue(assembly_q,t);
    	pthread_mutex_unlock(&assembly_mutex);
    } else if (random_type < 17) {
    	printf("type 4\n");
    	t.type = 4;
    	t.qa_task_id = qa_task_id;
    	pthread_mutex_lock(&painting_mutex);
    	Enqueue(painting_q,t);
    	pthread_mutex_unlock(&painting_mutex);
    	pthread_mutex_lock(&qa_mutex);
    	Enqueue(qa_q,t);
    	pthread_mutex_unlock(&qa_mutex);
    	pthread_mutex_lock(&qa_task_array_mutex);
    	qa_tasks[qa_task_id++];
    	pthread_mutex_unlock(&qa_task_array_mutex);
    } else if (random_type < 18) {
    	printf("type 5\n");
    	t.type = 5;
    	t.qa_task_id = qa_task_id;
    	pthread_mutex_lock(&assembly_mutex);
    	Enqueue(assembly_q,t);
    	pthread_mutex_unlock(&assembly_mutex);
    	pthread_mutex_lock(&qa_mutex);
    	Enqueue(qa_q,t);
    	pthread_mutex_unlock(&qa_mutex);
    	pthread_mutex_lock(&qa_task_array_mutex);
    	qa_tasks[qa_task_id++];
    	pthread_mutex_unlock(&qa_task_array_mutex);
    } else {
    	printf("no gift\n");
    	gift_id--;
    }
    
}


int main(int argc,char **argv){
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    
    init();
    create_threads();

    time(&start);
    while (check_time(start,end) < simulationTime) {
	create_request();
    }
    
    printf("Final look of queues: \n");
    print_all_queues();
    destroy_mutex();
    kill_threads();

    return 0;
}

void init() {
   srand(seed); // feed the seed
   gift_id = 1; // initialize gift ID's from 1
   qa_task_id = 0;
   t = 1; // unit time of simulation = 1 sec
   
   packaging_q = ConstructQueue(1000);
   painting_q = ConstructQueue(1000);
   assembly_q = ConstructQueue(1000);
   qa_q = ConstructQueue(1000);
   delivery_q = ConstructQueue(1000);
   
   pthread_mutex_init(&packaging_mutex,NULL);
   pthread_mutex_init(&painting_mutex,NULL);
   pthread_mutex_init(&assembly_mutex,NULL);
   pthread_mutex_init(&qa_mutex,NULL);
   pthread_mutex_init(&delivery_mutex,NULL);
   pthread_mutex_init(&qa_task_array_mutex, NULL);
}

void print_all_queues() {
   printf("-> Packaging Queue: ");
   pthread_mutex_lock(&packaging_mutex);
   packaging_q = print_queue(packaging_q);
   pthread_mutex_unlock(&packaging_mutex);
   printf("-> Painting Queue: ");
   pthread_mutex_lock(&painting_mutex);
   painting_q = print_queue(painting_q);
   pthread_mutex_unlock(&painting_mutex);
   printf("-> Assembly Queue: ");
   pthread_mutex_lock(&assembly_mutex);
   assembly_q = print_queue(assembly_q);
   pthread_mutex_unlock(&assembly_mutex);
   printf("-> QA Queue: ");
   pthread_mutex_lock(&qa_mutex);
   qa_q = print_queue(qa_q);
   pthread_mutex_unlock(&qa_mutex);
   printf("-> Delivery Queue: ");
   pthread_mutex_lock(&delivery_mutex);
   delivery_q = print_queue(delivery_q);
   pthread_mutex_unlock(&delivery_mutex);
}

void create_threads() {
   pthread_create(&tidA, NULL, ElfA, NULL);
   pthread_create(&tidB, NULL, ElfB, NULL);
   pthread_create(&tidS, NULL, Santa, NULL);
   pthread_create(&tidC, NULL, ControlThread, NULL);
}

void kill_threads() {
   pthread_kill(tidA, SIGTERM);
   pthread_kill(tidB, SIGTERM);
   pthread_kill(tidS, SIGTERM);
   pthread_kill(tidC, SIGTERM);
}

void destroy_mutex() {
   pthread_mutex_destroy(&packaging_mutex);
   pthread_mutex_destroy(&painting_mutex);
   pthread_mutex_destroy(&assembly_mutex);
   pthread_mutex_destroy(&delivery_mutex);
   pthread_mutex_destroy(&qa_mutex);
   pthread_mutex_destroy(&qa_task_array_mutex);
}

void send_for_packaging(Task t) {
   pthread_mutex_lock(&packaging_mutex);
   Enqueue(packaging_q, t);
   pthread_mutex_unlock(&packaging_mutex);
}

void send_for_delivery(Task t) {
   pthread_mutex_lock(&delivery_mutex);
   Enqueue(delivery_q, t);
   pthread_mutex_unlock(&delivery_mutex);
}

void* ElfA(void *arg) {
   Task current_task;
   while (1) {
   	pthread_mutex_lock(&packaging_mutex);
   	if (packaging_q->size > 0) {
   	    current_task = Dequeue(packaging_q);
   	    pthread_mutex_unlock(&packaging_mutex);
   	    pthread_sleep(t);
   	    printf("Gift: %d packaged by Elf A.\n",current_task.ID);
   	    send_for_delivery(current_task);
   	} else {
   	    pthread_mutex_unlock(&packaging_mutex);
   	    pthread_mutex_lock(&painting_mutex);
   	    if (painting_q->size > 0) {
   	        current_task = Dequeue(painting_q);
   	        pthread_mutex_unlock(&painting_mutex);
   	        pthread_sleep(3*t);
   	        printf("Gift: %d painted by Elf A.\n",current_task.ID);
   	        if (current_task.qa_task_id != -1) {
   	            pthread_mutex_lock(&qa_task_array_mutex);
   	            if (qa_tasks[current_task.qa_task_id]) {
   	               pthread_mutex_unlock(&qa_task_array_mutex); 
   	               send_for_packaging(current_task);
   	            } 
   	            else {
   	               qa_tasks[current_task.qa_task_id] = true;
   	               pthread_mutex_unlock(&qa_task_array_mutex); 
   	            }
   	        } else {
   	           send_for_packaging(current_task);
   	        }
   	    } else {
   	       pthread_mutex_unlock(&painting_mutex);
   	    }
   	}
   }
}

void* ElfB(void *arg){
   Task current_task;
   while (1) {
   	pthread_mutex_lock(&packaging_mutex);
   	if (packaging_q->size > 0) {
   	    current_task = Dequeue(packaging_q);
   	    pthread_mutex_unlock(&packaging_mutex);
   	    pthread_sleep(t);
   	    printf("Gift: %d packaged by Elf B.\n",current_task.ID);
   	    send_for_delivery(current_task);
   	} else {
   	    pthread_mutex_unlock(&packaging_mutex);
   	    pthread_mutex_lock(&assembly_mutex);
   	    if (assembly_q->size > 0) {
   	        current_task = Dequeue(assembly_q);
   	        pthread_mutex_unlock(&assembly_mutex);
   	        pthread_sleep(2*t);
   	        printf("Gift: %d assembled by Elf B.\n",current_task.ID);
   	        if (current_task.qa_task_id != -1) {
   	            pthread_mutex_lock(&qa_task_array_mutex);
   	            if (qa_tasks[current_task.qa_task_id]) {
   	               pthread_mutex_unlock(&qa_task_array_mutex); 
   	               send_for_packaging(current_task);
   	            } 
   	            else {
   	               qa_tasks[current_task.qa_task_id] = true;
   	               pthread_mutex_unlock(&qa_task_array_mutex); 
   	            }
   	        } else {
   	           send_for_packaging(current_task);
   	        }
   	    } else {
   	       pthread_mutex_unlock(&assembly_mutex);
   	    }
   	}
   }
}

// manages Santa's tasks
void* Santa(void *arg){
   Task current_task;
   while (1) {
      pthread_mutex_lock(&delivery_mutex);
      if (delivery_q->size > 0) {
         current_task = Dequeue(delivery_q);
         pthread_mutex_unlock(&delivery_mutex);
         pthread_sleep(t);
         printf("Gift: %d delivered by Santa.\n",current_task.ID);
      } else {
         pthread_mutex_unlock(&delivery_mutex);
         pthread_mutex_lock(&qa_mutex);
         if (qa_q->size > 0) {
            current_task = Dequeue(qa_q);
            pthread_mutex_unlock(&qa_mutex);
            pthread_sleep(t);
            printf("Gift: %d QA'd by Santa.\n",current_task.ID);
            pthread_mutex_lock(&qa_task_array_mutex);
            if (qa_tasks[current_task.qa_task_id]) {
               pthread_mutex_unlock(&qa_task_array_mutex);
               send_for_packaging(current_task);
            } else {
               qa_tasks[current_task.qa_task_id] = true;
               pthread_mutex_unlock(&qa_task_array_mutex);
            }
         } else {
            pthread_mutex_unlock(&qa_mutex);
         }
      }
   }
}

// the function that controls queues and output
void* ControlThread(void *arg){
   while (1) {
      pthread_sleep(10*t);
      print_all_queues();
   }
}
