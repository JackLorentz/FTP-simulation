#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<signal.h>
#include<time.h>
#include<sys/stat.h>

#define PORT 12345
#define MAXLINE 2048
#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)

struct FileInfo{
    bool is_owner_read;
    bool is_owner_write;
    bool is_group_read;
    bool is_group_write;
    bool is_other_read;
    bool is_other_write;
    int size;
    char creatorName[20];
    char creatorGroupName[20];
    char createdTime[100];
    char filename[100];
};

struct Capability{
    char filename[100];
    char ownername[100];
    bool is_owner_read;
    bool is_owner_write;
    bool is_read;
    bool is_write;
};

//處裡殭屍process(child process不再退出連線後會變成殭屍process, 以下用忽略的方式處理)
void handler(int sig);
void initialize(char* username, char* group, char* instr, char* filename, char* para, char* input, char* res);
int record2log(struct FileInfo *fileInfo);
int initializeLog(struct FileInfo *file_list, int* sync);
int getGroup(char* argv);
int getOperation(char* pch);
int createFile(char* filename, char*para, char* buf, struct FileInfo *fileInfo);
int readFile(char* filename, char*username, char* res, struct Capability* cap, int *sync);
int writeFile(char* filename, char* username, char* buf, char* para, char* res, struct Capability* cap, int *sync);
bool checkPermission(char* filename, char* username, struct Capability* cap, int *sync, int flag);
int changePermission(char* filename, char* username, int group, char* para, struct Capability* aos_list, struct Capability* cse_list, struct Capability* other_list, struct FileInfo *file_list, int *sync);
int updateLog(struct FileInfo* fileInfo, int* sync);
void show_message(char* username, char* group, int opt, char* filename, char* para, char* input);
int show_file_information(struct FileInfo* file_list, int* sync, char* filename, char* msg);
int updateLocalFiles(char* username, char* filename, char* para, char* buf);
void Wait(int* sync);
void Signal(int* sync);
int checkInstr(int opt, int instr_index);
int checkArguments(int opt, char* para);
int does_file_exist(char* filename, struct FileInfo *file_list, int* sync);
