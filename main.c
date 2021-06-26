#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>



#define MAX_STUDENT 250 
#define ROOMKEEPER_NUM 10 
#define ROOM_NUM 10 
#define ROOM_CAPACITY 4 

#define ANNOUNCING 0
#define CLEANING 1
#define BUSY 2

sem_t WaitingInRoom[ROOM_NUM];
sem_t WatingAtLibrary;
sem_t RoomKeepers[ROOM_NUM];
sem_t mutex;

int roomCapacitys[ROOM_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
int roomKeeperStates[ROOMKEEPER_NUM] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int serviceNumber[ROOMKEEPER_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int studentsCreated = 0, finishedStudy = 0;
int roomNo = 0;

void randwait(int secs) {
     int len = secs; // Generate an arbit number...
     sleep(len);
}

void simulation()
{
    int i = 0;
    int j = 0;

    printf("DEU LIBRARY \n");
    printf("------------------\n");

    for(i = 0; i < ROOM_NUM; i++)
    {
        printf("Room %d - ", i + 1);
        
        for(j = 0; j < roomCapacitys[i]; j++)
        {
            printf("X ");
        }

        for(j = 0; j < ROOM_CAPACITY - roomCapacitys[i]; j++)
        {
            printf("# ");
        }

        printf(" - ");
        printf("The room keeper is : ");

        if(roomKeeperStates[i] == CLEANING)
            printf("Cleaning ");
        else if(roomKeeperStates[i] == BUSY)
            printf("Busy, room is full ");
        else if(roomKeeperStates[i] == ANNOUNCING)
            printf("Announcing: Get up! Last %d seat. ", ROOM_CAPACITY - roomCapacitys[i]);

        //printf("| Total Transport Number: %d", serviceNumber[i]);
        
        printf("\n");
    }

    printf("Total Students Worked At Library: %d\n \n", finishedStudy);
    
}

void *student(void *studentId)
{
    int num = (int)studentId;
    int i = 0;
    int roomID;

    sem_wait(&mutex);
    studentsCreated++;
    sem_post(&mutex);


    printf("Student %d arrived at library.\n", num + 1);
    sem_wait(&WatingAtLibrary);
    sem_wait(&mutex);

    while (1)
    {
        int minStarvation = serviceNumber[0];
        int maxTotalSeat = 0;

        for(i = 1; i < ROOM_NUM; i++) 
        {
            if(serviceNumber[i] < serviceNumber[i - 1])
                minStarvation = serviceNumber[i];
        }
    
              
        for(i = 0; i < ROOM_NUM; i++) 
        {                             
                    
            if (roomKeeperStates[i] == BUSY || roomCapacitys[i] >= ROOM_CAPACITY)
            {
                continue;
            }

            if(roomCapacitys[i] > maxTotalSeat && roomCapacitys[i] < ROOM_CAPACITY){
                roomNo = i;  
                maxTotalSeat = roomCapacitys[i];            
            }
            else if(roomCapacitys[i] == maxTotalSeat && roomCapacitys[i] < ROOM_CAPACITY){
                if(serviceNumber[i] <= minStarvation){ 
                    roomNo = i;        
                }
            }
        }
      
        
              
     
        
        if (roomKeeperStates[roomNo] != BUSY && roomCapacitys[roomNo] < ROOM_CAPACITY )
        {
            roomID = roomNo;
            roomCapacitys[roomID]++;

            if(roomCapacitys[roomID] == ROOM_CAPACITY)
                serviceNumber[roomID]++;

            if(roomCapacitys[roomID] < ROOM_CAPACITY) 
                sem_post(&WatingAtLibrary); 

            break;
        }

    }

    if (roomKeeperStates[roomID] == CLEANING)
    {
        sem_post(RoomKeepers + roomID);
        sem_post(&mutex);
    }
    else
    {
        sem_post(&mutex);
    }

    sem_wait(&mutex);
    
    if (roomCapacitys[roomID] < ROOM_CAPACITY)
    {
        sem_post(&mutex);
        sem_wait(WaitingInRoom + roomID);
        sleep(1);
    }
    else if (roomCapacitys[roomID] == ROOM_CAPACITY)
    {
        sem_post(&mutex);
        sem_post(WaitingInRoom + roomID);
        sem_post(WaitingInRoom + roomID);
        sem_post(WaitingInRoom + roomID);
    }

    return NULL;
}

void *roomkeeper(void *roomkeeperID)
{
    int id = (int)roomkeeperID;
    int i = 0;

    while (1)
    {
        sem_post(&WatingAtLibrary);
        sem_wait(&mutex);
        roomKeeperStates[id] = ANNOUNCING;
        sem_post(&mutex);
        
        sem_wait(&mutex);
        int totalUnRooms = 0;

        for (i = 0; i < ROOM_NUM; i++)
        {
            totalUnRooms += roomCapacitys[i];
        }

        if (studentsCreated == 0 || (studentsCreated - finishedStudy - totalUnRooms == 0 && roomCapacitys[id] == 0)) 
        { 
            roomKeeperStates[id] = CLEANING;
            simulation();
            sem_post(&mutex);
            sem_wait(RoomKeepers + id);
            
        }
        else
        {
            sem_post(&mutex);
        }

        sem_wait(&mutex);
        roomKeeperStates[id] = ANNOUNCING;
        sem_post(&mutex);

        while (1)
        {
            sem_wait(&mutex);
            if (roomCapacitys[id] == ROOM_CAPACITY)
            {
                roomKeeperStates[id] = BUSY;
                sem_post(&mutex);
                break;
            }

            simulation(); 
            sem_post(&mutex);
            sleep(10);
        }

        randwait(1);

        sem_wait(&mutex);
        simulation(); 
        finishedStudy += roomCapacitys[id];
        roomCapacitys[id] = 0;
        
        sem_post(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int i, x, numStudent;
    printf("Maximum number of students can only be 250. Enter number of students: \n");
    scanf("%d",&x);
    numStudent = x;
    if (numStudent > MAX_STUDENT) {
        printf("The maximum number of Customers is %d.\n", MAX_STUDENT);
        system("PAUSE");   
        return 0;
    }

    pthread_t stid[MAX_STUDENT];
    pthread_t rktid[ROOMKEEPER_NUM];

    sem_init(&WatingAtLibrary, 0, 0);
    sem_init(&mutex, 0, 1);

    for (i = 0; i < ROOM_NUM; i++)
    {
        sem_init(RoomKeepers + i, 0, 0);
    }

    for (i = 0; i < ROOM_NUM; i++)
    {
        sem_init(WaitingInRoom + i, 0, 0);
    }

    for (i = 0; i < ROOMKEEPER_NUM; i++)
    {
        pthread_create(rktid + i, NULL, &roomkeeper, (void *)i);
    }

    for (i = 0; i < numStudent; i++)
    {
        pthread_create(stid + i, NULL, &student, (void *)i);
        randwait(rand() % 5);
    }

    for(i = 0; i < ROOMKEEPER_NUM; i++)
    {
        pthread_join(rktid[i], NULL);
    }

    for (i = 0; i < numStudent; i++)
    {
        pthread_join(stid[i], NULL);
    }

    return 0;
}
