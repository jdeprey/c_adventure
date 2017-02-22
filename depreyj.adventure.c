/*****************************************************************
 * Author: Joseph DePrey
 * Description:  adventure.c provides an interface for playing a game using
 *      the most recently generated rooms from buildrooms.c
 *  In the game, the player will begin in the "starting room" and will win
 *      the game automatically upon entering the "ending room", which causes the
 *      game to exit, displaying the path taken by the player.
 *  During the game, the player can also enter a command that returns the
 *      current time
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

// Buffer for max length of room names
#define NAME_LENGTH 10
// Constants provided by program specifications
#define NUM_ROOMS 7
// Max number of outgoing connections (amount is random) between each room
#define MAX_CONN 6
#define DIR_PREFIX "depreyj.rooms"
#define ROOM_ERROR "HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN."
#define TIME_FILENAME "currentTime.txt"
enum roomType {START_ROOM, MID_ROOM, END_ROOM};

// Room info
struct Room {
    char *name;
    enum roomType type;
    int connections[MAX_CONN];
};

// Global mutex for time thread
pthread_mutex_t time_mutex;

/******************Function Declarations**********************/
void getLatestRoomDir(char roomDir[]);
void readRooms(struct Room roomList[], char roomDir[], char *visited[]);
void gameOn(struct Room roomList[], char *visited[]);
void *writeTime();
void readTime();
/************************************************************
 *********************** MAIN *******************************
 *************************************************************/
int main(void){
    // Array of pointers to Rooms to hold data read from files
    struct Room roomList[NUM_ROOMS];
    // Array of Room names already read in from files
    char **visited;
    visited = malloc(NUM_ROOMS * sizeof(char*));
    for (int i = 0; i < NUM_ROOMS; i++){
        visited[i] = malloc((NAME_LENGTH+1) * sizeof(char));
    }
    // Get most recent room directory
    char roomDir[30];
    getLatestRoomDir(roomDir);
    // Read all files in room directory
    readRooms(roomList, roomDir, visited);
    // Finally, play the game
    gameOn(roomList, visited);
    return(0);
}

/************************************************************
 * Get most recent room directory from same directory as game
 *  Return directory name as pointer to char
 *************************************************************/
void getLatestRoomDir(char roomDir[]){
    DIR *d;
    // stat struct to hold directory info
    struct stat sb;
    // Used to sort folders by time
    int latest = 0;
    memset(roomDir, '\0', 30);
    struct dirent *dir;

    d = opendir("./");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Match dir names with our room dir prefix
            if(strstr(dir->d_name, DIR_PREFIX) != NULL){
                // Get dir stats
                stat(dir->d_name, &sb);
                // Find the most recently created dir
                if (((int) sb.st_mtime) > latest){
                    latest = (int) sb.st_mtime;
                    strcpy(roomDir, dir->d_name);
                }
            }
        }
    } else {
        printf("Directory is empty. Please run buildrooms program before playing.");
    }
    closedir(d);
}

/************************************************************
 * Read room data from files in room directory
 *  Store room data in array of pointers to Room structs
 *************************************************************/
void readRooms(struct Room roomList[], char roomDir[], char *visited[]){
    // Open latest room directory
    DIR *d;
    FILE *fp;
    int i, j;
    int roomCount = 0;

    char roomFilePath[100];
    memset(roomFilePath, '\0', strlen(roomFilePath));
    // Use these with fscanf because each file has only three fields per line
    //  e.g. ROOM, NAME, desert
    char field1[NAME_LENGTH];
    char field2[NAME_LENGTH];
    char field3[NAME_LENGTH];
    struct dirent *dir;
    d = opendir(roomDir);

    // Open all files in current Room directory and put filenames in array
    if (d) {
        // Make sure we get files, not . and ..
        readdir(d);
        readdir(d);
        printf("%s",roomDir);
        while((dir = readdir(d)) != NULL){
            // Put filenames in array for later use
            // memset(visited[roomCount], '\0', NAME_LENGTH);
            strcpy(visited[roomCount], dir->d_name);
            roomCount++;
        }
    } else {
        fprintf(stderr, "Unable to open dir %s\n", roomDir);
        exit(1);
    }

    // Open all named room files and get Room struct data
    for (i = 0; i < roomCount; i++){
        // Create path for room file by combining the current Room directory and filename
        sprintf(roomFilePath, "%s/%s", roomDir, visited[i]);
        // Open room file for reading
        fp = fopen(roomFilePath, "r");

        if (fp == NULL){
            fprintf(stderr, "Unable to open file %s\n", roomFilePath);
            exit(1);
        }

        // Get room name from file
        strcpy(roomList[i].name, visited[i]);
        // Set all connections to 0 (no connections by default)
        for (j = 0; j < NUM_ROOMS; j++){
            roomList[i].connections[j] = 0;
        }
        // Load connections and type from every room file
        //  Attempt to use fscanf to read data from room file
        while(fscanf(fp, "%s %s %s", field1, field2, field3) != EOF){
            if(strncmp(field2, "NAME", 4) == 0) {
                // Skip over the name value which we already have
               } else if(strncmp(field2, "TYPE", 4) == 0) {
                   if(strncmp(field3, "START_ROOM", 10) == 0) {
                       roomList[i].type = START_ROOM;
                   } else if(strncmp(field3, "MID_ROOM", 8) == 0) {
                       roomList[i].type = MID_ROOM;
                   } else if(strncmp(field3, "END_ROOM", 8) == 0) {
                       roomList[i].type = END_ROOM;
                   }
            }   else if (strncmp(field1, "CONNECTION", 10) == 0){
                // Set up room connections
                for (j = 0; j < NUM_ROOMS; j++){
                    if(strcmp(field3, visited[j]) == 0){
                        roomList[i].connections[j] = 1;
                    }
                }
            }
        }
        fclose(fp);
    }
    // Close room directory
    closedir(d);
}


/************************************************************
 * Start the game
 *************************************************************/
void gameOn(struct Room roomList[], char *visited[]){
    pthread_t time_thread;
    // Start Second Thread running
    pthread_create(&time_thread, NULL, writeTime, NULL);
    // Mutex lock it
    pthread_mutex_init(&time_mutex, NULL);
    pthread_mutex_lock(&time_mutex);

    // User input must be shorter than room names
    char buffer[NAME_LENGTH];
    // Current connection
    int currConn = 0;
    int i = 0;
    int j;
    int index = -1;
    // Set up default path
    int path[14] = {-1};
    int pathCount = 0;
    // Room struct for current room
    struct Room *currRoom = NULL;
    // Get START_ROOM
    do {
        if(roomList[i].type == START_ROOM) {
            currRoom = &roomList[i];
        }
        i++;
    } while((currRoom == NULL) && (i < NUM_ROOMS));

    // Let user continue looking for other rooms
    while(currRoom->type != END_ROOM){
        printf("CURRENT LOCATION: %s\n", currRoom->name);
        printf("POSSIBLE CONNECTIONS:");
        // Print connections
        // Reset to 0
        currConn = 0;
        for(j = 0; j < NUM_ROOMS; j++) {
            if(currRoom->connections[j] == 1) {
                if(currConn == 0) {
                    printf(" %s", visited[j]);
                } else {
                    printf(", %s", visited[j]);
                }
                currConn++;
            }
        }
        printf(".\n");
        Input:
        printf("WHERE TO? >");
        fgets(buffer, NAME_LENGTH, stdin);
        // Span the string until newline, set index at newline to null
        buffer[strcspn(buffer, "\n")] = 0;

        // Check if user entered valid Room name
        for(j = 0; j < NUM_ROOMS; j++) {
            if(strcmp(visited[j], buffer) == 0) {
                index = j;
            }
        }

        // Check if entered room is a connection
        if(currRoom->connections[index] == 1) {
            currRoom = &roomList[index];
            path[pathCount] = index;
            pathCount++;
        } else if (strcmp("time", buffer) == 0){
            // Unlock a mutex
            pthread_mutex_unlock(&time_mutex);
            // Join() on second thread
            pthread_join(time_thread, NULL);
            // writeTime();
            // Lock mutex again
            pthread_mutex_lock(&time_mutex);
            // Spawn second thread again
            pthread_create(&time_thread, NULL, writeTime, NULL);
            readTime();
            goto Input;
        } else {
            printf("\n%s\n", ROOM_ERROR);
        }
    }

    if(currRoom->type == END_ROOM) {
        printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
        printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", pathCount);

        for(i = 0; i < pathCount; i++) {
            printf("%s\n", visited[path[i]]);
        }
    }

    // Kill second (time) thread
    pthread_detach(time_thread);
    pthread_cancel(time_thread);

}

/************************************************************
 * Write current time to currentTime.txt file
 *************************************************************/
void *writeTime(){
    // Attempt to lock mutex
    pthread_mutex_lock(&time_mutex);

    // Time variables
    time_t rawtime;
    struct tm *info;
    char timeBuffer[80] = {0};
    // Output file pointer
    FILE *fp = NULL;

    time(&rawtime);
    info = localtime(&rawtime);
    strftime(timeBuffer, 80, "%l:%M%p, %A, %B %d, %Y", info);

    fp = fopen(TIME_FILENAME, "w");
    if (fp == NULL){
        fprintf(stderr, "Unable to open file %s for writing.\n", TIME_FILENAME);
        exit(1);
    }
    fprintf(fp, "%s", timeBuffer);
    fclose(fp);

    // Unlock mutex
    pthread_mutex_unlock(&time_mutex);
    return NULL;
}

/************************************************************
 * Read current time from currentTime.txt file and print it
 *************************************************************/
void readTime(){
    FILE *fp2 = NULL;
    // Time test
    char line[80];

    fp2 = fopen(TIME_FILENAME, "r");

    if (fp2 < 0){
        fprintf(stderr, "Unable to open file %s for reading.\n", TIME_FILENAME);
        exit(1);
    }

    while (fgets(line, sizeof(line), fp2) != NULL) {
        printf("\n%s\n", line);
    }
    fclose(fp2);
}