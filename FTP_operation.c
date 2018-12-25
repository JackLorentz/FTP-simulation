#include"FTP_operation.h"
//處裡殭屍process(child process不再退出連線後會變成殭屍process, 以下用忽略的方式處理)
void handler(int sig){
    printf("\t[FTP Server] recv a sig=%d\n", sig);
    exit(EXIT_SUCCESS);
}

void initialize(char* username, char* group, char* instr, char* filename, char* para, char* input, char* res){
    memset(username, 0, 20);
    memset(group, 0, 5);
    memset(instr, 0, MAXLINE);
    memset(filename, 0, MAXLINE);
    memset(para, 0, MAXLINE);
    memset(input, 0, MAXLINE);
    memset(res, 0, MAXLINE);
}

int record2log(struct FileInfo *fileInfo){
    FILE* fptr = fopen("Log", "a");
    if(fptr == NULL){
        return -1;
    }
    else{
        fwrite(fileInfo, sizeof(struct FileInfo), 1, fptr);
        //printf("%s\n", fileInfo->filename);
    }
    fclose(fptr);
    return 0;
}

int initializeLog(struct FileInfo *file_list, int* sync){
    FILE* fptr = fopen("Log", "r");
    if(fptr == NULL){
        return -1;
    }
    else{
        while(fread((file_list + *sync), sizeof(struct FileInfo), 1, fptr)){
            //printf("%s\n", (*(capability_list + *sync)).filename);
            *sync = *sync + 1;
        }
    }
    fclose(fptr);
    return 0;
}

int getGroup(char* argv){
    if(strstr(argv, "AOS") - argv == 0){
        return 0;
    }
    else if(strstr(argv, "CSE") - argv == 0){
        return 1;
    }
    else{
        return 2;
    }
}

int getOperation(char* pch){
    //搜尋子字串, 回傳開始一樣的位置
    if(strstr(pch, "new") - pch == 0 && strlen(pch) == 3){
        return 0;
    }
    else if(strstr(pch, "read") - pch == 0 && strlen(pch) == 4){
        return 1;
    }
    else if(strstr(pch, "write") - pch == 0 && strlen(pch) == 5){
        return 2;
    }
    else if(strstr(pch, "change") - pch == 0 && strlen(pch) == 6){
        return 3;
    }
    else if(strstr(pch, "information") - pch == 0 && strlen(pch) == 11){
        return 4;
    }

    return -1;
}

void show_message(char* username, char* group, int opt, char* filename, char* para, char* input){
    printf("[FTP Server] Get Message: \n");
    printf("---------------------------------\n");
    printf("User: %s\n", username);
    printf("Group: %s\n", group);
    printf("Operation: %d\n", opt);
    printf("File name: %s\n", filename);
    printf("parameter: %s\n", para);
    printf("content:\n%s", input);
    printf("---------------------------------\n");
}

int createFile(char* filename, char* para, char* buf, struct FileInfo *fileInfo){
    char dir_name[100];
    //取得當前檔案目錄
    getcwd(dir_name, sizeof(dir_name));
    strcat(dir_name, "/FTP/");
    strcat(dir_name, filename);
    printf("[FTP Server] File Path: %s\n", dir_name);
    FILE* fptr = fopen(dir_name, "w");
    if(fptr == NULL){
        return -1;
    }
    else{
        fwrite(buf, 1, strlen(buf), fptr);
    }
    fclose(fptr);
    //
    time_t now;
    time(&now);
    strcpy(fileInfo->filename, filename);
    fileInfo->size = sizeof(buf);
    strcpy(fileInfo->createdTime, ctime(&now));
    if(fileInfo->createdTime[strlen(fileInfo->createdTime) - 1] == '\n'){
        fileInfo->createdTime[strlen(fileInfo->createdTime) - 1] = '\0';
    }
    //owner
    if(para[0] == 'r'){
        fileInfo->is_owner_read = true;
    }
    else{
        fileInfo->is_owner_read = false;
    }
    if(para[1] == 'w'){
        fileInfo->is_owner_write = true;
    }
    else{
        fileInfo->is_owner_write = false;
    }
    //group
    if(para[2] == 'r'){
        fileInfo->is_group_read = true;
    }
    else{
        fileInfo->is_group_read = false;
    }
    if(para[3] == 'w'){
        fileInfo->is_group_write = true;
    }
    else{
        fileInfo->is_group_write = false;
    }
    //other
    if(para[4] == 'r'){
        fileInfo->is_other_read = true;
    }
    else{
        fileInfo->is_other_read = false;
    }
    if(para[5] == 'w'){
        fileInfo->is_other_write = true;
    }
    else{
        fileInfo->is_other_write = false;
    }

    return 0;
}

int does_file_exist(char* filename, struct FileInfo *file_list, int* sync){
    for(int i=0; i<*sync; i++){
        if(strstr((*(file_list + i)).filename, filename) - (*(file_list + i)).filename == 0){
            return 1;
        }
    }
    return 0;
}

int show_file_information(struct FileInfo *file_list, int* sync, char* filename, char* msg){
    int i;
    for(i=0; i<*sync; i++){
        if(strstr((*(file_list + i)).filename, filename) - (*(file_list + i)).filename == 0){
             //AOS
            if((*(file_list + i)).is_owner_read){
                printf("-r");
                strcat(msg, "-r");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            if((*(file_list + i)).is_owner_write){
                printf("w-");
                strcat(msg, "w-");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            //CSE
            if((*(file_list + i)).is_group_read){
                printf("-r");
                strcat(msg, "-r");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            if((*(file_list + i)).is_group_write){
                printf("w-");
                strcat(msg, "w-");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            //other
            if((*(file_list + i)).is_other_read){
                printf("-r");
                strcat(msg, "-r");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            if((*(file_list + i)).is_other_write){
                printf("w-");
                strcat(msg, "w-");
            }
            else{
                printf("--");
                strcat(msg, "--");
            }
            printf("  ");
            strcat(msg, "  ");
            //Name
            printf("%s  ", (*(file_list + i)).creatorName);
            strcat(msg, (*(file_list + i)).creatorName);
            strcat(msg, "  ");
            //Group
            printf("%s  ", (*(file_list + i)).creatorGroupName);
            strcat(msg, (*(file_list + i)).creatorGroupName);
            strcat(msg, "  ");
            //bytes
            printf("%d  ", (*(file_list + i)).size);
            //整數轉字串
            char s[MAXLINE];
            sprintf(s, "%d", (*(file_list + i)).size);
            strcat(msg, s);
            strcat(msg, "  ");
            //date
            printf("%s  ", (*(file_list + i)).createdTime);
            strcat(msg, (*(file_list + i)).createdTime);
            strcat(msg, "  ");
            //filename
            printf("%s\n", (*(file_list + i)).filename);
            strcat(msg, (*(file_list + i)).filename);
            return 0;
        }
    }
    return -1;
}

int readFile(char* filename, char*username, char* res, struct Capability* cap, int *sync){
    if(!checkPermission(filename, username, cap, sync, 0)){
        return -1;
    }
    char dir_name[100];
    char buf[MAXLINE];
    getcwd(dir_name, sizeof(dir_name));
    strcat(dir_name, "/FTP/");
    strcat(dir_name, filename);
    FILE* fptr = fopen(dir_name, "r");
    if(fptr == NULL){
        return 0;
    }
    else{
        while((fread(buf, 1, sizeof(buf), fptr)) > 0){
            strcat(res, buf);
        }
        //printf("[FTP Server] Content (Read):\n%s\n", res);
    }
    fclose(fptr);
    return 1;
}

int writeFile(char* filename, char* username, char* buf, char* para, char* res, struct Capability* cap, int *sync){
    if(!checkPermission(filename, username, cap, sync, 1)){
         return -1;
    }
    char dir_name[100];
    getcwd(dir_name, sizeof(dir_name));
    strcat(dir_name, "/FTP/");
    strcat(dir_name, filename);
    
    FILE* fptr = NULL;
    //override
    if(para[0] == 'o'){
        fptr = fopen(dir_name, "w");
    }
    //append
    else if(para[0] == 'a'){
        fptr = fopen(dir_name, "a");
    }

    if(fptr == NULL){
        return 0;
    }
    else{
        fwrite(buf, 1, strlen(buf), fptr);
        strcat(res, buf);
        //printf("[FTP Server] Content (Write):\n%s\n", res);
    }
    fclose(fptr);
    return 1;
}

//flag = 0 => check read permission, flag = 1 => check write permission
bool checkPermission(char* filename, char* username, struct Capability* cap, int *sync, int flag){
    for(int i=0; i<*sync; i++){
        //strstr(source, target): 在source字串中尋找跟target匹配的子字串, 並回傳開始符合位置的指標
        if(strstr((*(cap + i)).filename, filename) - (*(cap + i)).filename == 0){
            //如果owner是用戶本人
            if(strstr((*(cap + i)).ownername, username) - (*(cap + i)).ownername == 0){
                if(flag == 0){
                    return (*(cap + i)).is_owner_read;
                }
                else{
                    return (*(cap + i)).is_owner_write;
                }
            }
            else{
                if(flag == 0){
                    return (*(cap + i)).is_read;
                }
                else{
                    return (*(cap + i)).is_write;
                }
            }
            break;
        }
    }
    return false;
}

int changePermission(char* filename, char* username, int group, char* para, struct Capability* aos_list, struct Capability* cse_list, struct Capability* other_list, struct FileInfo *file_list, int *sync){
    for(int i=0; i<*sync; i++){
        if(strstr((*(file_list + i)).filename, filename) - (*(file_list + i)).filename == 0){
            //先判斷是否為本人
            if(strstr((*(file_list + i)).creatorName, username) - (*(file_list + i)).creatorName != 0 || strlen((*(file_list + i)).creatorName) != strlen(username)){
                break;
            }
            //修改權限
            //Owner
            if(para[0] == 'r'){
                (*(aos_list + i)).is_owner_read = true;
                (*(cse_list + i)).is_owner_read = true;
                (*(other_list + i)).is_owner_read = true;
                (*(file_list + i)).is_owner_read = true;
            }
            else{
                (*(aos_list + i)).is_owner_read = false;
                (*(cse_list + i)).is_owner_read = false;
                (*(other_list + i)).is_owner_read = false;
                (*(file_list + i)).is_owner_read = false;
            }
            if(para[1] == 'w'){
                (*(aos_list + i)).is_owner_write = true;
                (*(cse_list + i)).is_owner_write = true;
                (*(other_list + i)).is_owner_write = true;
                (*(file_list + i)).is_owner_write = true;
            }
            else{
                (*(aos_list + i)).is_owner_write = false;
                (*(cse_list + i)).is_owner_write = false;
                (*(other_list + i)).is_owner_write = false;
                (*(file_list + i)).is_owner_write = false;
            }
            //Group
            //AOS
            if(group == 0){
                //same group
                if(para[2] == 'r'){
                    (*(aos_list + i)).is_read = true;
                    (*(file_list + i)).is_group_read = true;
                }
                else{
                    (*(aos_list + i)).is_read = false;
                    (*(file_list + i)).is_group_read = false;
                }
                if(para[3] == 'w'){
                    (*(aos_list + i)).is_write = true;
                    (*(file_list + i)).is_group_write = true;
                }
                else{
                    (*(aos_list + i)).is_write = false;
                    (*(file_list + i)).is_group_write = false;
                }
                //other
                if(para[4] == 'r'){
                    (*(cse_list + i)).is_read = true;
                    (*(other_list + i)).is_read = true;
                    (*(file_list + i)).is_other_read = true;
                }
                else{
                    (*(cse_list + i)).is_read = false;
                    (*(other_list + i)).is_read = false;
                    (*(file_list + i)).is_other_read = false;
                }
                if(para[5] == 'w'){
                    (*(cse_list + i)).is_write = true;
                    (*(other_list + i)).is_write = true;
                    (*(file_list + i)).is_other_write = true;
                }
                else{
                    (*(cse_list + i)).is_write = false;
                    (*(other_list + i)).is_write = false;
                    (*(file_list + i)).is_other_write = false;
                }
            }
            //CSE
            else if(group == 1){
                //same group
                if(para[2] == 'r'){
                    (*(cse_list + i)).is_read = true;
                    (*(file_list + i)).is_group_read = true;
                }
                else{
                    (*(cse_list + i)).is_read = false;
                    (*(file_list + i)).is_group_read = false;
                }
                if(para[3] == 'w'){
                    (*(cse_list + i)).is_write = true;
                    (*(file_list + i)).is_group_write = true;
                }
                else{
                    (*(cse_list + i)).is_write = false;
                    (*(file_list + i)).is_group_write = false;
                }
                //other
                if(para[4] == 'r'){
                    (*(aos_list + i)).is_read = true;
                    (*(other_list + i)).is_read = true;
                    (*(file_list + i)).is_other_read = true;
                }
                else{
                    (*(aos_list + i)).is_read = false;
                    (*(other_list + i)).is_read = false;
                    (*(file_list + i)).is_other_read = false;
                }
                if(para[5] == 'w'){
                    (*(aos_list + i)).is_write = true;
                    (*(other_list + i)).is_write = true;
                    (*(file_list + i)).is_other_write = true;
                }
                else{
                    (*(aos_list + i)).is_write = false;
                    (*(other_list + i)).is_write = false;
                    (*(file_list + i)).is_other_write = false;
                }
            }
            //OTHER
            else{
                //same group
                if(para[2] == 'r'){
                    (*(other_list + i)).is_read = true;
                    (*(file_list + i)).is_group_read = true;
                }
                else{
                    (*(other_list + i)).is_read = false;
                    (*(file_list + i)).is_group_read = false;
                }
                if(para[3] == 'w'){
                    (*(other_list + i)).is_write = true;
                    (*(file_list + i)).is_group_write = true;
                }
                else{
                    (*(other_list + i)).is_write = false;
                    (*(file_list + i)).is_group_write = false;
                }
                //other
                if(para[4] == 'r'){
                    (*(aos_list + i)).is_read = true;
                    (*(cse_list + i)).is_read = true;
                    (*(file_list + i)).is_other_read = true;
                }
                else{
                    (*(aos_list + i)).is_read = false;
                    (*(cse_list + i)).is_read = false;
                    (*(file_list + i)).is_other_read = false;
                }
                if(para[5] == 'w'){
                    (*(aos_list + i)).is_write = true;
                    (*(cse_list + i)).is_write = true;
                    (*(file_list + i)).is_other_write = true;
                }
                else{
                    aos_list[i].is_write = false;
                    cse_list[i].is_write = false;
                    file_list[i].is_other_write = false;
                }
            }

            /*if(updateLog(file_list, sync) == -1){
                printf("[FTP Server] Updating Log file is failed !\n");
            }
            else{
                printf("[FTP Server] Updating Log file successed !\n");
            }*/
            return 0;
        }
    }
    return -1;
}

int updateLog(struct FileInfo* file_list, int* sync){
    FILE* fptr = fopen("Log", "w");
    
    if(fptr == NULL){
        return -1;
    }
    else{
        int cnt = 0;
        while(cnt < *sync){
            //printf("%s\n", (*(capability_list + *sync)).creatorName);
            fwrite((file_list + cnt), sizeof(struct FileInfo), 1, fptr);
            cnt++;
        }
    }
    fclose(fptr);

    return 0;
}

int updateLocalFiles(char* username, char* filename, char* para, char* buf){
    char dir_name[100];
    getcwd(dir_name, sizeof(dir_name));
    strcat(dir_name, "/");
    strcat(dir_name, username);
    strcat(dir_name, "/");
    //建立用戶自己的資料夾
    //先確認目錄是否存在
    if(access(dir_name, 0)){
        if(mkdir(dir_name, 0777) == -1){
            return -1;
        }
        printf("[FTP Client] Creating user directory successed !\n");
    }
    
    FILE* fptr = NULL;
    strcat(dir_name, filename);
    //append
    if(para[0] == 'a' && strlen(para) > 0){
        fptr = fopen(dir_name, "a");
    }
    //override
    else{
        fptr = fopen(dir_name, "w");
    }

    if(fptr == NULL){
        return 0;
    }
    else{
        fwrite(buf, 1, strlen(buf), fptr);
    }
    fclose(fptr);
    return 1;
}

void Wait(int* sync){
    while(*sync <= 0){}
    *sync = *sync - 1;
}

void Signal(int* sync){
    *sync = *sync + 1;
}

int checkInstr(int opt, int instr_index){
    switch(opt){
        //new
        case 0:
        //write
        case 2:
        //change
        case 3:
            if(instr_index < 3){
                return 1;
            }
            else if(instr_index > 3){
                return 2;
            }
            return 0;
            break;
        //read
        case 1:
         //information
        case 4:
            if(instr_index < 2){
                return 1;
            }
            else if(instr_index > 2){
                return 2;
            }
            return 0;
        default:    break;
    }
    return -1;
}

int checkArguments(int opt, char* para){
    switch(opt){
        //new, change
        case 0:
        case 3:
            //參數太長
            if(strlen(para) > 6){
                return 2;
            }
            else if(strlen(para) < 6){
                return 3;
            }
            //參數符號錯誤
            for(int i=0; i<strlen(para); i++){
                if(para[i] != 'r' && para[i] != 'w' && para[i] != '-'){
                    return 1;
                }
            }
            return 0;
        //write
        case 2:
            //參數太長
            if(strlen(para) > 1){
                return 2;
            }
            //參數符號錯誤
            for(int i=0; i<strlen(para); i++){
                if(para[i] != 'a' && para[i] != 'o'){
                    return 1;
                }
            }
            return 0;
        default:    break;
    }
    return -1;
}