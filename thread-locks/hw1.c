#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

typedef struct counter_t counter_t;
struct counter_t {
    int value;
    pthread_mutex_t lock;
};

static void init(counter_t *c) 
{
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

static void incr(counter_t *c)
{
    pthread_mutex_lock(&c->lock);
    c->value++;
    pthread_mutex_unlock(&c->lock);
}

static void decr(counter_t *c)
{
    pthread_mutex_lock(&c->lock);
    c->value--;
    pthread_mutex_unlock(&c->lock);
}

static int get(counter_t *c)
{
    pthread_mutex_lock(&c->lock);
    int rc = c->value;
    pthread_mutex_unlock(&c->lock);
    return rc;
}

static void* incr_count(void *arg)
{
    counter_t *cp = (counter_t *)arg;
    static const int end = 1e6;
    for(int i = 0; i < end; i++)
        incr(cp);
    return NULL;
}

#define MAX_THREADS (1)
int main(int argc, char *argv[])
{
    int num_threads = MAX_THREADS;
    if (argc > 1) {
        num_threads = strtoul(argv[1], 0, 10);
    }

    void *ret;
    pthread_t *tid = calloc(num_threads, sizeof(tid[0]));
    counter_t c;
    struct timeval tv1, tv2, res;

    init(&c);
    gettimeofday(&tv1, NULL);
    for(int i = 0; i < num_threads; i++) {
        pthread_create(&tid[i], NULL, incr_count, &c);
        pthread_join(tid[i], &ret);
    }
    gettimeofday(&tv2, NULL);

   timersub(&tv2, &tv1, &res);
   printf("secs: %ld sussecs: %d\n", res.tv_sec, res.tv_usec);
   free(tid);
    return 0;
}
