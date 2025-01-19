#include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
    #include <string.h>
    
    #define MAX_STUDENTS 10
    #define MAX_RESOURCE_TYPES 3
    #define INITIAL_RESOURCES 2
    
    typedef struct {
        int available;
        int max_available;
        char name[50];
    } Resource;
    
    typedef struct {
        Resource resources[MAX_RESOURCE_TYPES];
        int waiting_threads;
    
        pthread_mutex_t mutex;
        pthread_cond_t can_allocate;
    
        int total_operations;
    } Monitor;
    
    void monitor_init(Monitor* m) {
        pthread_mutex_init(&m->mutex, NULL);
        pthread_cond_init(&m->can_allocate, NULL);
    
        char* names[] = {"Resursa_A", "Resursa_B", "Resursa_C"};
        int max_amounts[] = {INITIAL_RESOURCES, INITIAL_RESOURCES, INITIAL_RESOURCES};
    
        for (int i = 0; i < MAX_RESOURCE_TYPES; i++) {
            m->resources[i].available = max_amounts[i];
            m->resources[i].max_available = max_amounts[i];
            strcpy(m->resources[i].name, names[i]);
        }
    
        m->waiting_threads = 0;
        m->total_operations = 0;
    }
    
    void monitor_request(Monitor* m, int student_id, int resource_id, int amount) {
        pthread_mutex_lock(&m->mutex);
    
        printf("Student %d încearcă să folosească %d %s\n",
               student_id, amount, m->resources[resource_id].name);
    
        while (m->resources[resource_id].available < amount) {
            m->waiting_threads++;
            printf("Student %d așteaptă %d %s\n",
                   student_id, amount, m->resources[resource_id].name);
            pthread_cond_wait(&m->can_allocate, &m->mutex);
            m->waiting_threads--;
        }
    
        if (m->resources[resource_id].available >= amount) {
            m->resources[resource_id].available -= amount;
            m->total_operations++;
            printf("Student %d folosește %d %s\n",
                   student_id, amount, m->resources[resource_id].name);
        }
    
        pthread_mutex_unlock(&m->mutex);
    }
    
    void monitor_release(Monitor* m, int student_id, int resource_id, int amount) {
        pthread_mutex_lock(&m->mutex);
    
        m->resources[resource_id].available += amount;
        printf("Student %d a eliberat %d %s\n",
               student_id, amount, m->resources[resource_id].name);
    
        if (m->waiting_threads > 0) {
            pthread_cond_broadcast(&m->can_allocate);
        }
    
        pthread_mutex_unlock(&m->mutex);
    }
    
    void change_resource_limits(Monitor* m, int new_limit) {
        pthread_mutex_lock(&m->mutex);
        printf("\n=== Modificare limită resurse la %d ===\n", new_limit);
    
        for (int i = 0; i < MAX_RESOURCE_TYPES; i++) {
            m->resources[i].max_available = new_limit;
            if (m->resources[i].available > new_limit) {
                m->resources[i].available = new_limit;
            }
        }
    
        pthread_mutex_unlock(&m->mutex);
    }
    
    void monitor_status(Monitor* m) {
        pthread_mutex_lock(&m->mutex);
    
        printf("\n=== Stare Monitor ===\n");
        printf("Studenți în așteptare: %d\n", m->waiting_threads);
        printf("Total operații: %d\n", m->total_operations);
    
        for (int i = 0; i < MAX_RESOURCE_TYPES; i++) {
            printf("%s: %d/%d disponibile\n",
                   m->resources[i].name,
                   m->resources[i].available,
                   m->resources[i].max_available);
        }
    
        pthread_mutex_unlock(&m->mutex);
    }
    
    void* student(void* arg) {
        int student_id = *((int*)arg);
        Monitor* m = (Monitor*)((void**)arg)[1];
    
        while (1) {
            int resource = rand() % MAX_RESOURCE_TYPES;
            int amount = 1; 
    
            printf("Student %d încearcă să folosească un %s\n",
                   student_id, m->resources[resource].name);
    
            monitor_request(m, student_id, resource, amount);
    
            printf("Student %d folosește %s\n",
                   student_id, m->resources[resource].name);
            sleep(rand() % 3 + 1);
    
            monitor_release(m, student_id, resource, amount);
    
            sleep(rand() % 2 + 1);
        }
        return NULL;
    }
    
    int main() {
        Monitor monitor;
        pthread_t threads[MAX_STUDENTS];
        int student_ids[MAX_STUDENTS];
    
        srand(time(NULL));
        monitor_init(&monitor);
    
        for (int i = 0; i < MAX_STUDENTS; i++) {
            student_ids[i] = i + 1;
            void** args = malloc(2 * sizeof(void*));
            args[0] = &student_ids[i];
            args[1] = &monitor;
            pthread_create(&threads[i], NULL, student, args);
        }
    
        monitor_status(&monitor);
        sleep(5);
    
        printf("\n=== Test 1: Creștere resurse la 4 ===\n");
        change_resource_limits(&monitor, 4);
        monitor_status(&monitor);
        sleep(5);
    
        printf("\n=== Test 2: Reducere resurse la 1 ===\n");
        change_resource_limits(&monitor, 1);
        monitor_status(&monitor);
        sleep(5);
    
        printf("\n=== Test 3: Revenire la 2 resurse ===\n");
        change_resource_limits(&monitor, 2);
        monitor_status(&monitor);
        sleep(5);
    
        for (int i = 0; i < MAX_STUDENTS; i++) {
            pthread_cancel(threads[i]);
            pthread_join(threads[i], NULL);
        }
    
        pthread_mutex_destroy(&monitor.mutex);
        pthread_cond_destroy(&monitor.can_allocate);
    
        return 0;
    }
     
       
    
