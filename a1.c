#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

void parseDir(char *path){
    DIR *dir;
    struct dirent *entry;
    if(!(dir = opendir(path))){
        printf("womp womp\n");
    }
    else{
        while((entry = readdir(dir)) != NULL){
            if(entry->d_type == DT_DIR)
                if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
                       printf("%s %s\n",path, entry->d_name);
				else{
					char path2[1024];
					strcat(path2, path);
					strcat(path2,"/");
					strcat(path2, entry->d_name);
					printf("%s\n",path2);				
				}
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]){
    if(argc == 2){
        parseDir(argv[1]);
    }
    return 0;
}
