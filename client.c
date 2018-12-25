#include"FTP_operation.h"
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<signal.h>

int main(int argc, char *argv[]){
    //用戶訊息
    char username[20];
    char groupname[5];
    //
    struct sockaddr_in server;  
    int socketfd;  
    //傳出訊息
    char msg[MAXLINE];
    char instr[100];
    char input[MAXLINE];
    //本地建立檔案
    char *delim = " ";
    char *pch;
    int opt = -1;
    int instr_index = 0;
    char filename[100];
    char para[100];
    //接收訊息
    char buf[MAXLINE];  
    int n_bytes;  
    //設定用戶資訊
    for(int i=0; i<argc; i++){
        if(i == 1){
            strcpy(username, argv[i]);
        }
        else if(i == 2){
            strcpy(groupname, argv[i]);
            break;
        }
    }
    printf("[FTP Client] Hi, %s! You are in %s group.\n", username, groupname);
    printf("[FTP Client] >> ");
    while((fgets(instr, MAXLINE, stdin)) != NULL){
        //初始化訊息
        memset(msg, 0, sizeof(msg));
        strcpy(msg, username);
        strcat(msg, " ");
        strcat(msg, groupname);
        strcat(msg, " ");
        //接收指令
        if(instr[strlen(instr) - 1] == '\n'){
            instr[strlen(instr) - 1] = '\0';
        }
        //若命令打"exit"則跳出shell
        if(strstr(instr, "exit") - instr == 0){
            printf("[FTP Client] EXIT\n");
            break;
        }
        strcat(msg, instr);
        //剖析命令: 重設置命令與參數
        opt = -1;
        memset(para, 0, sizeof(para));
        memset(filename, 0, sizeof(filename));
        instr_index = 0;
        pch = strtok(instr, delim);
        while(pch != NULL){
            switch(instr_index){
                case 0://操作指令
                    opt = getOperation(pch);
                    break;
                case 1://檔名
                    strcpy(filename, pch);
                    break;
                case 2://參數
                    strcpy(para, pch);
                    break;
                default:    break;
            }
            instr_index ++;
            pch = strtok(NULL, delim);
        }
        //檢查指令參數數目
        if(checkInstr(opt, instr_index) == -1){
            printf("[FTP Client] You have wrong command !\n");
            printf("[FTP Client] >> ");
            continue;
        }
        else if(checkInstr(opt, instr_index) == 1){
            printf("[FTP Client] Too few arguments to command.\n");
            printf("[FTP Client] >> ");
            continue;
        }
        else if(checkInstr(opt, instr_index) == 2){
            printf("[FTP Client] Too many arguments to command.\n");
            printf("[FTP Client] >> ");
            continue;
        }
        //檢查指令參數內容
        if(checkArguments(opt, para) == 1){
            printf("[FTP Client] Your 3rd argument symbols are incorrect.\n");
            printf("[FTP Client] >> ");
            continue;
        }
        else if(checkArguments(opt, para) == 2){
            printf("[FTP Client] Too many symbols to your 3rd argument.\n");
            printf("[FTP Client] >> ");
            continue;
        }
        else if(checkArguments(opt, para) == 3){
            printf("[FTP Client] Too few symbol to your 3rd argument.\n");
            printf("[FTP Client] >> ");
            continue;
        }
        //若是在Server建檔或上傳, 要先寫些內容
        memset(input, 0, sizeof(input));
        if(opt == 0 || opt == 2){
            printf("(Write Something) ");
            fgets(input, MAXLINE, stdin);
            strcat(msg, " ");
            strcat(msg, input);
            if(opt == 2){
                //在本地也建立檔案
                int stat = updateLocalFiles(username, filename, para, input);
                if(stat == -1){
                    printf("[FTP Client] Creating local directory failed !\n");
                    printf("[FTP Client] >> ");
                    continue;
                }
                else if(stat == 0){
                    printf("[FTP Client] Creating local file failed !\n");
                    printf("[FTP Client] >> ");
                    continue;
                }
            }
        }
        //show_message(username, groupname, opt, filename, para, input);
        /* 製作 socket */  
        socketfd = socket(AF_INET, SOCK_STREAM, 0);  
        /* 準備連線端指定用的 struct 資料 */  
        server.sin_family = AF_INET;  
        server.sin_port = htons(PORT);  
        /* 127.0.0.1 是 localhost 本機位址 */  
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr.s_addr);  
        /* 與 server 端連線 */  
        connect(socketfd, (struct sockaddr *)&server, sizeof(server));  
        //發送訊息給Server
        if(send(socketfd, msg, strlen(msg), 0) == -1){
            perror("send");
		    exit (EXIT_SUCCESS);
        }
        //printf("[FTP Client] Sending message successed !\n");
        /* 從Server接受資料 */  
        memset(buf, 0, sizeof(buf));  
        while((n_bytes = recv(socketfd, buf, MAXLINE, 0)) == -1){}
        //若是下載檔案
        if(opt == 1){
            if(buf[0] == '0' && strlen(buf) == 1){
                printf("[FTP Client] Permission Denied or The File doesn't exist.\n");
            }
            else if(buf[1] == '1' && strlen(buf) == 1){
                printf("[FTP Client] The File doesn't exist.\n");
            }
            else{
                //在本地也建立檔案
                int stat = updateLocalFiles(username, filename, para, buf);
                if(stat == -1){
                    printf("[FTP Client] Creating local directory failed !\n");
                }
                else if(stat == 0){
                    printf("[FTP Client] Creating local file failed !\n");
                }
                printf("%s\n", buf);
            }
        }
        else{
            printf("%s\n", buf);
        }
        /* 結束 socket */  
        close(socketfd);
        printf("[FTP Client] >> ");
    }
    return 0;
}