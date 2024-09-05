#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define NUM_CPUS 4

typedef struct counter_t counter_t;
struct counter_t {
    int global;
    pthread_mutex_t glock;
    int local[NUM_CPUS];
    pthread_mutex_t lock[NUM_CPUS];
    int threshold;
};

static void init(counter_t *c, int threshold) 
{
    c->global = 0;
    c->threshold = threshold;
    pthread_mutex_init(&c->glock, NULL);
    for(int i = 0; i < NUM_CPUS; i++) {
        c->local[i] = 0;
        pthread_mutex_init(&c->lock[i], NULL);
    }
}

static void update(counter_t *c, int threadId, int amt)
{
    int cpu = threadId % NUM_CPUS;
    pthread_mutex_lock(&c->lock[cpu]);
    c->local[cpu] += amt;
    if(c->local[cpu] >= c->threshold) {
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu];
        pthread_mutex_unlock(&c->glock);
        c->local[cpu] = 0;
    }
    pthread_mutex_unlock(&c->lock[cpu]);
}

static int get(counter_t *c)
{
    pthread_mutex_lock(&c->glock);
    int ret = c->global; 
    pthread_mutex_unlock(&c->glock);
    return ret;
}

struct thread_ctxt {
    int threadid;
    counter_t *cp;
};

static void* incr_count(void *arg)
{
    struct thread_ctxt *cxt = (struct thread_ctxt*)arg;
    static const int end = 1e7;
    for(int i = 0; i < end; i++)
        update(cxt->cp, (cxt->threadid), 1);
    return NULL;
}

#define MAX_THREADS (1)
int main(int argc, char *argv[])
{
    int num_threads = MAX_THREADS;
    if (argc > 1) {
        num_threads = strtoul(argv[1], 0, 10);
    }

    int s = 2;
    if (argc > 2) {
        s = strtoul(argv[2], 0, 10);
    }

    void *ret;
    pthread_t *tid = calloc(num_threads, sizeof(tid[0]));
    counter_t c;
    struct timeval tv1, tv2, res;

    init(&c, s);
    gettimeofday(&tv1, NULL);
    for(int i = 0; i < num_threads; i++) {
        struct thread_ctxt tc = {
            .threadid = i,
            .cp = &c,
        };

        pthread_create(&tid[i], NULL, incr_count, &tc);
        pthread_join(tid[i], &ret);
    }
    gettimeofday(&tv2, NULL);

   timersub(&tv2, &tv1, &res);
   printf("secs: %ld sussecs: %d\n", res.tv_sec, res.tv_usec);
   free(tid);
    return 0;
}
