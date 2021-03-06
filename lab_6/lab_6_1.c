#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>


struct th_args
{
    char flag; 
    sem_t* sem_r;
    sem_t* sem_w;
    int* addr;                                                                                                                                                                                                                                                                                                                                                               
};


void* th_func(void* args)
{
    char* flag = &((struct th_args* ) args)->flag;
    sem_t* sem_r = ((struct th_args* ) args)->sem_r;
    sem_t* sem_w = ((struct th_args* ) args)->sem_w;
    int* addr = ((struct th_args* ) args)->addr;

    int num = 0;
    while((int *)(*flag) != 10)
    {
        memcpy(addr, &num, sizeof(num));
        printf("Writer: %d\n", num);
        
        sem_post(sem_w);
        sem_wait(sem_r);
        sleep(1);
        
        num++;
    }   
}


int main()
{
    errno = 0;
    struct th_args params;
    pthread_t th;

    int fd = shm_open("shared_memory", O_CREAT|O_RDWR, 0644);
    ftruncate(fd, sizeof(int));
    params.addr = (int*)mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    params.sem_r = sem_open("sem_read", O_CREAT, 0644, 0);
    params.sem_w = sem_open("sem_write", O_CREAT, 0644, 0);
    
    if(pthread_create(&th, NULL, th_func, (void*)(&params)))
    {
        printf("Thread create failed with error: %s\n", strerror(errno));
        return -errno;
    }
       
    params.flag = getchar();

    void* status;
    int res = pthread_join(th, &status);
    if (res != 0)
    {
        printf("Thread join failed with error: %s\n", strerror(res));
        return -errno;
    }
    
    munmap(0, sizeof(int));
    close(fd);
    sem_close(params.sem_r);
    sem_close(params.sem_w);
    shm_unlink("shared_memory");
    sem_unlink("sem_read");
    sem_unlink("sem_write");
    
    return 0;
}