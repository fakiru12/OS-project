#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

void parseDir(char *path, int x){
    DIR *dir;
    struct dirent *entry;
    if(!(dir = opendir(path))){
        printf("womp womp\n");
    }
    else{
        while((entry = readdir(dir)) != NULL){
            if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0){
                for(int i = 0; i < x; i++)
                    printf("| ");
                printf("|_");
                printf("%s\n", entry->d_name);
                if(entry->d_type == DT_DIR){
                    char path2[1024] = "";
                    strcat(path2,path);
                    strcat(path2,"/");
                    strcat(path2,entry->d_name);
                    parseDir(path2,x+1);   
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]){
    if(argc >= 3){
        int x = atoi(argv[1]);
        for(int i = 1; i <= x; i++){
            printf("%s\n",argv[1 + i]);
            parseDir(argv[1 + i],0);
            printf("\n");
        }
    }
    return 0;
}