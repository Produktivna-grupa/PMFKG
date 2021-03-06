#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>

#define H 10

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array; 
};

void validate_return_value(int retrunValue) {
    if (retrunValue == -1) {
        perror("unexpected error occurred");
        exit(1);
    }
}

int main() {

    key_t key = ftok("./osoba.c", 'a');
    validate_return_value(key);

    int semnum = 3;
    int semflg = 0666 | IPC_CREAT | IPC_EXCL;
    int semid = semget(key, semnum, semflg);

    int shmflg = 0666 | IPC_CREAT;
    int shmid = shmget(key, sizeof(int), shmflg);
    validate_return_value(shmid);

    int *coins = (int*) shmat(shmid, NULL, 0);

    if (semid == -1) {
        if (errno == EEXIST) {
            semid = semget(key, semnum, 0);
        } else {
            validate_return_value(semid);
        }
    } else {
        union semun sem_init;
        sem_init.array = (unsigned short*)malloc(semnum * sizeof(unsigned short));
        
        sem_init.array[0] = 1; // mutex za sinhronizaciju osoba
        sem_init.array[1] = 0; // osoba javlja detetu
        sem_init.array[2] = 0; // dete javlja nazad osobi

        int rv = semctl(semid, 0, SETALL, sem_init);
        validate_return_value(rv);

        (*coins) = 0;
    }

    struct sembuf mutex_inc, mutex_dec, child_inc, child_dec;

    mutex_dec.sem_num = 0;
    mutex_dec.sem_op = -1;
    mutex_dec.sem_flg = 0;

    mutex_inc.sem_num = 0;
    mutex_inc.sem_op = 1;
    mutex_inc.sem_flg = 0;

    child_inc.sem_num = 1;
    child_inc.sem_op = 1;
    child_inc.sem_flg = 0;

    child_dec.sem_num = 2;
    child_dec.sem_op = -1;
    child_dec.sem_flg = 0;

    int rv = semop(semid, &mutex_dec, 1);
    validate_return_value(rv);

    // samo jedna osoba izvrsava

    (*coins)++;
    if ((*coins) == H) {
        rv = semop(semid, &child_inc, 1);
        validate_return_value(rv);

        rv = semop(semid, &child_dec, 1);
        validate_return_value(rv);
    }

    printf("Ubacio novcic... trenutno u kasici %d...\n", *coins);

    //

    rv = semop(semid, &mutex_inc, 1);
    validate_return_value(rv);

    rv = shmdt(coins);
    validate_return_value(rv);

    return 0;
}