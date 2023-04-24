#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>

#define SHM_SIZE 400

typedef struct book {
    int id;
    int row;
    int bookshelf;
    int place;
} book;

int shm_fd;
sem_t *sem;
book *catalog;

void toclean() {
    sem_close(sem);
    sem_unlink("/sem");
    shm_unlink("/shared_memory");
    close(shm_fd);
}

void sigint_handler(int signum) {
    toclean();
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if (argc < 3) {
        printf("not enough arguments: there are %d, need 3\n", argc);
        exit(1);
    }
    
    signal(SIGINT, sigint_handler);
    
    // получаем данные
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);

    // создание семафора
    if ((sem = sem_open("/sem", O_CREAT, 0666, 0)) == SEM_FAILED) {
        perror("Error sem_open");
        exit(1);
    }

    // создание разделяемой памяти
    if ((shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666)) == -1) {
        perror("Error shm_open");
        exit(1);
    }
    
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("Error ftruncate");
        exit(1);
    }
    
    if ((catalog = (book *) mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
        perror("Error mmap");
        exit(1);
    }

    // дочерние процессы
    pid_t p;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            p = fork();
            if (p == -1) {
                perror("Error fork()");
                exit(1);
            } else if (p == 0) {
                for (int l = 0; l < k; ++l) {
                    int index = i * n * k + j * k + l;
                    catalog[index].row = i;
                    catalog[index].bookshelf = j;
                    catalog[index].place = l;
                    catalog[index].id = index;
                }
                sem_post(sem);
                exit(0);
            }
        }
    }
    
    
    // ждем завершение процессов
    for (int i = 0; i < m; ++i) {
        wait(NULL);
    }

    // сортировка каталога
    for (int i = 0; i < m * n * k; ++i) {
        for (int j = i + 1; j < m * n * k; j++) {
            if (catalog[i].id > catalog[j].id) {
                book tmp = catalog[i];
                catalog[i] = catalog[j];
                catalog[j] = tmp;
            }
        }
    }
    
    // вывод
    printf("Books in catalog: \n");
    printf("id: rows: bookshelves: books:\n");
    for (int i = 0; i < m * n * k; ++i) {
        printf("%d %d %d %d\n", catalog[i].id, catalog[i].row, catalog[i].bookshelf, catalog[i].place);
    }
    printf("\n");

    toclean();
    return 0;
}
