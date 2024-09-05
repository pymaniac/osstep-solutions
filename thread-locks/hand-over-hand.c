#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct node_t node_t;

struct node_t {
    int val;
    pthread_mutex_t lock;
    struct node_t *next;
};

node_t *head; // sentinent

static node_t* alloc_node(int value)
{
    node_t *new_node = calloc(1, sizeof(new_node[0]));
    if(!new_node) return NULL;
    new_node->val = value;
    pthread_mutex_init(&new_node->lock,  NULL);
    return new_node;
}

static void delete_node(node_t *pnode)
{
    if (pthread_mutex_destroy(&pnode->lock))
        printf("pthread_mutex_destroy faiiled: node->val: %d, err: %m", pnode->val);
    free(pnode);
}

static void print_nodes(void)
{
    
    for(node_t *curr = head; curr != NULL; curr = curr->next) {
        printf("%d\t", curr->val);
    }
    printf("\n");
}

static void add_node(int value)
{
    node_t *new_node = alloc_node(value);

    pthread_mutex_lock(&head->lock);
    new_node->next = head->next;
    head->next = new_node;
    pthread_mutex_unlock(&head->lock);
}

static int del_node(int value)
{
    node_t  *prev = head;
    pthread_mutex_lock(&prev->lock);
    node_t *curr = prev->next;
    if(!curr) goto done;

    pthread_mutex_lock(&curr->lock);
    while(curr != NULL) {
//        printf("Deleting value: %d curr->val %d\n", value, curr->val);
        if(curr->val == value) {
            prev->next = curr->next;
            break;
        }
        pthread_mutex_unlock(&prev->lock);
        prev = curr;
        curr = prev->next;
//        printf("\tcurr: %p\n", curr);
        if(curr)
            pthread_mutex_lock(&curr->lock);
    }

    int ret = -1; // value not preset
    if (curr) {
        pthread_mutex_unlock(&curr->lock);
 //       printf("Deleting curr: %d\n", curr->val);
        delete_node(curr);
        ret = 0; // value present
    }
    else {
   //     printf("Node curr: %d not found\n", value);
    }
done:
    pthread_mutex_unlock(&prev->lock);
    return ret;
}
typedef struct thread_info thread_info;

struct thread_info {
    pthread_t tid;
    int thread_num;
};

static void* thread_driver(void *arg)
{
    thread_info *ti = arg;

    // odd threads are add threads, even are delete threads
    // delete threads delete with a  random delay, start with a delay to
    // allow add threads to add
    if(ti->thread_num > 0) {
        printf("Deleting nodes %d\n", ti->thread_num);
        for(int i = 0; i < 100; i++) {
            del_node(i*2+400*(ti->thread_num-1));
        }
    }
    else {
        printf("Adding nodes %d\n", ti->thread_num);
        for(int i = 0; i < 200; i++) {
            add_node(i*2+400*ti->thread_num);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int num_threads = 40;
    thread_info *tinfo;

    tinfo = calloc(num_threads,  sizeof(tinfo[0]));

    head =  alloc_node(-1);
    for(int i = 0; i < num_threads; i++) {
        tinfo[i].thread_num = i;
        pthread_create(&tinfo[i].tid, NULL, thread_driver,  &tinfo[i]);
    }

    for(int i = 0; i < num_threads; i++) {
        void *res;
        pthread_join(tinfo[i].tid, &res);
    }

    printf("Threads done\n");
    node_t *curr = head;
    while(curr) {
        node_t *next = curr->next;
        delete_node(curr);
        curr = next;
    }

    return 0;
}

