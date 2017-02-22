/*****************************************************************
 * Author: Joseph DePrey
 * Description:  buildrooms.c creates a series of files that hold
 *  descriptions of in-game rooms and how the rooms are connected
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Constants provided by program specifications
#define NUM_ROOMS 7
// Number of hard-coded room names
#define NAME_POOL 10
// Buffer for max length of room names
#define NAME_LENGTH 10
// Minimum number of outgoing connections between from each room to other rooms
#define MIN_CONN 3
// Max number of outgoing connections (amount is random) between each room
#define MAX_CONN 6

enum roomType {START_ROOM, MID_ROOM, END_ROOM};

const char *rooms[NAME_POOL] = {
    "desert",
    "shop",
    "castle",
    "field",
    "forest",
    "village",
    "mountain",
    "temple",
    "lake",
    "valley",
};

// Room info
struct Room {
    const char *name;
    enum roomType type;
    int connections[MAX_CONN];
};

// Array of room structs for writing and reading
struct Room roomList[NUM_ROOMS];

/******************Function Declarations**********************/
void getRandomList(int size, int *randoms);
int getRandomInt(int m, int n);
void createDir(char *roomDir);
void generateRooms(struct Room roomList[], char roomDir[], int *randomList);

/************************************************************
 *********************** MAIN *******************************
 *************************************************************/
int main(void){

    // Output file directory name should be less than 32 chars
    char roomDir[32] = {0};
    // Array of random numbers
    int randoms[NUM_ROOMS] = {0};
    // Seed random number generator
    srand(time(NULL));

    // Create directory of Room files
    createDir(roomDir);
    // cd to Room file directory
    chdir(roomDir);
    // Generate 7 unique random numbers between 0 and 10 to get Room names
    getRandomList(NAME_POOL, randoms);
    // Generate room files
    generateRooms(roomList, roomDir, randoms);
    chdir("..");
    return 0;
}


/************************************************************
 * Returns array of unique random ints between 0 and size, inclusive
 *************************************************************/
void getRandomList(int size, int *randoms){
    int list[NAME_POOL];
    int i, j;
    // Create list of numbers from 0 to 9
    for (i = 0; i < NAME_POOL; i++){
        list[i] = i;
    }
    // Shuffle list using rand(), then take values from it
    for (i = 0; i < size; i++) {
        j = i + rand() % (NAME_POOL - i);
        int temp = list[i];
        list[i] = list[j];
        list[j] = temp;

        randoms[i] = list[i];
    }
}

/************************************************************
 * Get random int between m and n inclusive
 *************************************************************/
int getRandomInt(int m, int n){
    return (rand() % (n - m + 1) + m);
}

/************************************************************
 * Create directory for room files to be stored in
 *************************************************************/
void createDir(char *roomDir){
    // Create output file dir name as "ONID username + .rooms. + current PID"
    sprintf(roomDir, "%s.%d", "depreyj.rooms", getpid());
    // Create room file directory, which will not be removed after execution
    mkdir(roomDir, 0755);
}


/************************************************************
 * Generate rooms and output data to files in room directory
 *************************************************************/
void generateRooms(struct Room roomList[], char roomDir[], int *randomList){
    // Output file pointer
    FILE *fp = NULL;
    int i, j, random;
    int connects, counter;

    // Fill initial array with 7 Room structs
    for (i = 0; i < NUM_ROOMS; i++){
        // Give unique name to each room by iterating through pre-generated array of random ints
        roomList[i].name = rooms[randomList[i]];
        // Set all room types to MID_ROOM by default (easier to change later)
        roomList[i].type = MID_ROOM;
        // Set all connections to 0 (no connections by default)
        for (j = 0; j < NUM_ROOMS; j++){
            roomList[i].connections[j] = 0;
        }
    }

    // Setup START and END type rooms.  There is only one of each, so they will be
    //  chosen at random
    roomList[0].type = START_ROOM;
    random = getRandomInt(1, 6);
    roomList[random].type = END_ROOM;

    // Create connections between rooms
    for (i = 0; i < NUM_ROOMS; i++){
        // Number of connections between each Room
        connects = 0;

        // Create at least 3 connections between each Room
        while (connects < MIN_CONN){
            // Get random numbers to connect to.  Number cannot be same as Room index
            do {
                random = getRandomInt(0, 6);
            } while (random == i);

            roomList[i].connections[random] = 1;
            roomList[random].connections[i] = 1;

            connects++;
        }

    }

    // Write file to Room directory.  Each file contains only one Room struct
    // Make file for each Room
    for (i = 0; i < NUM_ROOMS; i++){
        // Open file with write permissions
        fp = fopen(roomList[i].name, "w");
        if (fp < 0){
            fprintf(stderr, "Unable to create file.");
            exit(1);
        }
        // Write the room name
        fprintf(fp, "ROOM NAME: %s\n", roomList[i].name);
        // Write the connections
        counter = 1;
        for (j = 0; j < NUM_ROOMS; j++){
            // Check if rooms are connected
            if(roomList[i].connections[j] == 1){
                fprintf(fp, "CONNECTION %d: %s\n", counter, roomList[j].name);
                counter++;
            }
        }
        // Write the room type
        fprintf(fp, "ROOM TYPE: ");

        if (roomList[i].type == START_ROOM){
            fprintf(fp, "START_ROOM");
        } else if (roomList[i].type == MID_ROOM){
            fprintf(fp, "MID_ROOM");
        } else {
            fprintf(fp, "END_ROOM");
        }
        // Add newline to end of file and close current Room file
        fprintf(fp, "\n");
        fclose(fp);
    }
}