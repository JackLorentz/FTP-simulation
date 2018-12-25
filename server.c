#include"FTP_operation.h"
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

typedef struct FileInfo FileInfo;
typedef struct Capability Capability;
  
int main(void)  
{ 
    //以下為共享變數
    //Capability list 
    Capability *aos_list;
    Capability *cse_list;
    Capability *other_list;
    //File Information
    FileInfo *file_list;
    int *sync;
    //建立Shared Memory
    int mmfd = open("file_list", O_CREAT|O_RDWR|O_TRUNC, 00777);
    int aosfd = open("aos_capability_list", O_CREAT|O_RDWR|O_TRUNC, 00777);
    int csefd = open("cse_capability_list", O_CREAT|O_RDWR|O_TRUNC, 00777);
    int otherfd = open("other_capability_list", O_CREAT|O_RDWR|O_TRUNC, 00777);
    //同步處理變數
    int syncfd = open("syncVar", O_CREAT|O_RDWR|O_TRUNC, 00777);
    if(mmfd == -1 || aosfd == -1 || csefd == -1 || otherfd == -1 || syncfd == -1){
        printf("[Shared Memory] Error opening file for accessing\n");
        exit(EXIT_FAILURE);
    }
    //配置區塊大小: 把指標移動最後面
    if(lseek(mmfd, sizeof(FileInfo)*100 - 1, SEEK_SET) == -1 || 
        lseek(aosfd, sizeof(Capability)*100 - 1, SEEK_SET) == -1 || 
        lseek(csefd, sizeof(Capability)*100 - 1, SEEK_SET) == -1 || 
        lseek(otherfd, sizeof(Capability)*100 - 1, SEEK_SET) == -1 ||
        lseek(syncfd, sizeof(int)*10 - 1, SEEK_SET) == -1){
        printf("[Shared Memory] lseek error\n");
        exit(EXIT_FAILURE);
    }
    //配置區塊大小: 在最後面標上'\0'
    if(write(mmfd, "", 1) == -1 || write(aosfd, "", 1) == -1 || write(csefd, "", 1) == -1 || write(otherfd, "", 1) == -1 || write(syncfd, "", 1) == -1){
        printf("[Shared Memory] Error writing last byte of the file\n");
        exit(EXIT_FAILURE);
    }

    file_list = (FileInfo*)mmap(NULL, sizeof(FileInfo)*100, PROT_READ|PROT_WRITE, MAP_SHARED, mmfd, 0);
    aos_list = (Capability*)mmap(NULL, sizeof(Capability)*100, PROT_READ|PROT_WRITE, MAP_SHARED, aosfd, 0);
    cse_list = (Capability*)mmap(NULL, sizeof(Capability)*100, PROT_READ|PROT_WRITE, MAP_SHARED, csefd, 0);
    other_list = (Capability*)mmap(NULL, sizeof(Capability)*100, PROT_READ|PROT_WRITE, MAP_SHARED, otherfd, 0);
    sync = (int*)mmap(NULL, sizeof(int)*10, PROT_READ|PROT_WRITE, MAP_SHARED, syncfd, 0);
    
    if(file_list == MAP_FAILED || aos_list == MAP_FAILED || cse_list == MAP_FAILED || other_list == MAP_FAILED || sync == MAP_FAILED){
        close(mmfd);
        close(aosfd);
        close(csefd);
        close(otherfd);
        close(syncfd);
        printf("[Shared Memory] Error mmapping the file\n");
        exit(EXIT_FAILURE);
    }
    close(mmfd);
    close(aosfd);
    close(csefd);
    close(otherfd);
    close(syncfd);
    //sync第0個位置紀錄fileInfo的共享記憶體offset
    *sync = 0;
    //semaphore: wrt => wirte操作的互斥鎖
    *(sync + 1) = 1;
    //semaphore: readcnt => 計數操作read的process(reader)數
    *(sync + 2) = 0;
    //semaphore: mutex => 保護readcnt的互斥鎖
    *(sync + 3) = 1;
    //User information
    char username[20];
    char group[5];
    //instruction
    char instr[MAXLINE];
    char filename[MAXLINE];
    char para[MAXLINE];
    char input[MAXLINE];
    char *delim = " ";
    char *pch;
    int instr_index = 0;
    int opt = -1;
    //
    int client_num = 0;
    int socketfd;  
    struct sockaddr_in addr;  
    struct sockaddr_in client;  
    socklen_t len;  
    int socket_client;  
    int n_bytes;
    char response[MAXLINE];

    //載入現有FTP的檔案資料
    /*if(initializeLog(file_list, sync) == -1){
        printf("[FTP Server] Loading files is failed !\n");
    }
    else{
        printf("[FTP Server] Loading files successed !\n");
    }*/
  
    /* 製作 socket */  
    socketfd = socket(AF_INET, SOCK_STREAM, 0);  
    /* 設定 socket */  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(PORT);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    bind(socketfd, (struct sockaddr*)&addr, sizeof(addr)); 
    printf("[FTP Server] binding...\n");  
  
    /* 設定成等待來自 TCP 用戶端的連線要求狀態 */  
    listen(socketfd, 5);  
    printf("[FTP Server] listening...\n");  
  
    /* 接受來自 TCP 用戶端地連線要求*/  
    printf("[FTP Server] wait for connection...\n");  
    len = sizeof(client);  
    //multiprocess接收clients訊息
    pid_t pid;
    while(1){
        if((socket_client = accept(socketfd, (struct sockaddr *)&client, &len)) < 0){
            ERR_EXIT("accept error");
        }  
        //接收訊息
        //初始化buffer使其為空
        initialize(username, group, instr, filename, para, input, response);
        if((n_bytes = recv(socket_client, instr, MAXLINE, 0)) == -1){
            perror("recv");
            exit(1);
        }
        //剖析命令: 重設置命令與參數
        instr_index = 0;
        pch = strtok(instr, delim);
        while(pch != NULL){
            switch(instr_index){
                case 0://用戶名稱
                    strcpy(username, pch);
                    break;
                case 1://所屬群組
                    strcpy(group, pch);
                    break;
                case 2://操作指令
                    opt = getOperation(pch);
                    break;
                case 3://檔名
                    strcpy(filename, pch);
                    break;
                case 4://參數
                    strcpy(para, pch);
                    break;
                case 5://寫入內容
                    strcpy(input, pch);
                    break;
                default:    break;
            }
            instr_index ++;
            pch = strtok(NULL, delim);
        }
        show_message(username, group, opt, filename, para, input);
        //
        client_num ++;
        //產生child process處理client命令 
        pid = fork();
        if(pid == -1){
            ERR_EXIT("fork error");
        }
        //child process
        else if(pid == 0){
            //處裡殭屍process
            signal(SIGUSR1, handler);
            /* 結束 listen 的 socket */  
            printf("[FTP Server] Close self connection...\n");
            close(socketfd);
            //printf("[FTP Server] %d clients online\n", client_num);
            //
            int stat = -1;
            switch(opt){
                //new
                case 0:
                    /*printf("\tname: %s\n", capability_list[list_index]->creatorName);
                    system("pause");*/
                    //若有用戶new一個FTP上同檔名檔案, 則回傳建檔失敗
                    if(does_file_exist(filename, file_list, sync) == 1){
                        printf("[FTP Server] The file exists.\n");
                        strcpy(response, "The file exists.\n");
                    }
                    else{
                        if(createFile(filename, para, input, (file_list + *sync)) == -1){
                            printf("[FTP Server] Creating file is failed!\n");
                            strcpy(response, "Creating file is failed!\n");
                        }   
                        else{
                            //設定檔案資訊
                            strcpy((*(file_list + *sync)).creatorName, username);
                            strcpy((*(file_list + *sync)).creatorGroupName, group);
                            //紀錄
                            /*if(record2log(file_list + *sync) == 0){
                                printf("[FTP Server] Recording to log file successed !\n");
                            }
                            else{
                                printf("[FTP Server] Recording to log file is failed !\n");
                            }*/
                            //新增至capability list
                            if(getGroup((*(file_list + *sync)).creatorGroupName) == 0){
                                strcpy((*(aos_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(aos_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(aos_list + *sync)).is_read = (*(file_list + *sync)).is_group_read;
                                (*(aos_list + *sync)).is_write = (*(file_list + *sync)).is_group_write;
                                (*(aos_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(aos_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;

                                strcpy((*(cse_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(cse_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(cse_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(cse_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(cse_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(cse_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;

                                strcpy((*(other_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(other_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(other_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(other_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(other_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(other_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                            }
                            else if(getGroup((*(file_list + *sync)).creatorGroupName) == 1){
                                strcpy((*(cse_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(cse_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(cse_list + *sync)).is_read = (*(file_list + *sync)).is_group_read;
                                (*(cse_list + *sync)).is_write = (*(file_list + *sync)).is_group_write;
                                (*(cse_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(cse_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                                
                                strcpy((*(aos_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(aos_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(aos_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(aos_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(aos_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(aos_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                               
                                strcpy((*(other_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(other_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(other_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(other_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(other_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(other_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                            }
                            else{
                                strcpy((*(other_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(other_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(other_list + *sync)).is_read = (*(file_list + *sync)).is_group_read;
                                (*(other_list + *sync)).is_write = (*(file_list + *sync)).is_group_write;
                                (*(other_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(other_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                                
                                strcpy((*(aos_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(aos_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(aos_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(aos_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(aos_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(aos_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                                
                                strcpy((*(cse_list + *sync)).filename, (*(file_list + *sync)).filename);
                                strcpy((*(cse_list + *sync)).ownername, (*(file_list + *sync)).creatorName);
                                (*(cse_list + *sync)).is_read = (*(file_list + *sync)).is_other_read;
                                (*(cse_list + *sync)).is_write = (*(file_list + *sync)).is_other_write;
                                (*(cse_list + *sync)).is_owner_read = (*(file_list + *sync)).is_owner_read;
                                (*(cse_list + *sync)).is_owner_write = (*(file_list + *sync)).is_owner_write;
                            }
                            //顯示
                            printf("[FTP Server] Creating file:\n");
                            strcpy(response, "Creating file:\n");
                            *sync = *sync + 1;
                            if(show_file_information(file_list, sync, filename, response) == -1){
                                printf("[FTP Server] The file doesn't exist.\n");
                                strcpy(response, "[FTP Server] The file doesn't exist.\n");
                            }
                        }
                    }
                    break;
                //read
                case 1:
                    Wait(sync + 3);
                    //CS start (readcnt的CS)
                    *(sync + 2) = *(sync + 2) + 1;
                    //若這是第一個reader
                    if(*(sync + 2) == 1){
                        //把writer擋下, 讓之後的reader優先進來CS
                        Wait(sync + 1);
                    }
                    //CS end
                    Signal(sync + 3);
                    //CS start
                    //strcpy(response, "File Content(Read):\n");
                    if(getGroup(group) == 0){
                        stat = readFile(filename, username, response, aos_list, sync);
                    }
                    else if(getGroup(group) == 1){
                        stat = readFile(filename, username, response, cse_list, sync);
                    }
                    else{
                        stat = readFile(filename, username, response, other_list, sync);
                    }
                    //
                    if(stat == -1){
                        strcpy(response, "0");
                    }
                    else if(stat == 0){
                        strcpy(response, "1");
                    }
                    sleep(20);
                    //CS end
                    *(sync + 2) = *(sync + 2) - 1;
                    //若readcnt為0, 表示已無reader可以讓writer進來CS
                    if(*(sync + 2) == 0){
                        Signal(sync + 1);
                    }
                    break;
                //write
                case 2:
                    Wait(sync + 1);
                    //CS start
                    //strcpy(response, "File Content(Write):\n");
                    if(getGroup(group) == 0){
                        stat = writeFile(filename, username, input, para, response, aos_list, sync);
                    }
                    else if(getGroup(group) == 1){
                        stat = writeFile(filename, username, input, para, response, cse_list, sync);
                    }
                    else{
                        stat = writeFile(filename, username, input, para, response, other_list, sync);
                    }
                    //
                    if(stat == -1){
                        strcpy(response, "Permission Denied or The File doesn't exist.");
                    }
                    else if(stat == 0){
                        strcpy(response, "The File doesn't exist.");
                    }
                    sleep(20);
                    //CS end
                    Signal(sync + 1);
                    break;
                //change
                case 3:
                    if(changePermission(filename, username, getGroup(group), para, aos_list, cse_list, other_list, file_list, sync) == -1){
                        strcpy(response, "The File doesn't exist or Permission denied.");
                    }
                    else{
                        strcpy(response, "Permission changing successed !");
                    }
                    break;
                //information
                case 4:
                    if(show_file_information(file_list, sync, filename, response) == -1){
                         printf("[FTP Server] The file doesn't exist.\n");
                         strcpy(response, "[FTP Server] The file doesn't exist.\n");
                    }
                    break;
                default:    
                    strcpy(response, "You have wrong command !");
                    break;
            }
            /* 回傳訊息給client */  
            printf("[FTP Server] Response to client:\n%s\n", response);  
            if(send(socket_client, response, strlen(response), 0) == -1){
                perror("send");
		        exit (EXIT_SUCCESS);
            }
            //取消映射
            munmap(file_list, sizeof(FileInfo)*100);
            munmap(aos_list, sizeof(Capability)*100);
            munmap(cse_list, sizeof(Capability)*100);
            munmap(other_list, sizeof(Capability)*100);
            munmap(sync, sizeof(int)*10);
            printf("[FTP Server] Unmap shared memory.\n");
            exit(EXIT_SUCCESS);
        }
        //parent process
        else{

            /* 結束 TCP 對話 */  
            printf("[FTP Server] Close client connection...\n");  
            close(socket_client);  
        }
    }
    //
    return 0;  
}