#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define SNAPSHOT_FILE "snapshot.txt"

void write_snapshot_file(const char *path) {
    FILE *snapshot_fp;
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char filepath[1024];

    dir = opendir(path);
    if (dir == NULL) {
        printf("Failed to open directory: %s\n", path);
        return;
    }

    snprintf(filepath, sizeof(filepath), "%s/%s", path, SNAPSHOT_FILE);
    snapshot_fp = fopen(filepath, "w");
    if (snapshot_fp == NULL) {
        printf("Failed to create snapshot file for directory: %s\n", path);
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
        if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            fprintf(snapshot_fp, "%s\t%ld\n", entry->d_name, (long)file_stat.st_mtime);
        }
    }

    fclose(snapshot_fp);
    closedir(dir);
}

int snapshot_file_exists(const char *path) {
    struct stat st;
    char filepath[1024];

    snprintf(filepath, sizeof(filepath), "%s/%s", path, SNAPSHOT_FILE);

    if (stat(filepath, &st) == 0) 
        return 1;
     else 
        return 0;
}

void parseDir(char *path, int x){
    DIR *dir;
    struct dirent *entry;
    if(!(dir = opendir(path))){
        printf("Failed to open directory: %s\n", path);
    }
    else{
        if (!snapshot_file_exists(path)) 
            write_snapshot_file(path); 
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
