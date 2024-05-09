#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#define SNAPSHOT_FILE "snapshot.txt"

#define MAX_LINE_LENGTH 1000

typedef struct files {
    char name[101];
    struct files* next; 
} files;//the file structure

typedef struct dirs {
    char name[101];
    char path[101];
    struct dirs* next;
    files* file_head;
} dirs;//the dir structure

dirs* root[10] = {NULL};
int cnt=0;
    void replace_char(char *str, char old_char, char new_char);
    char* last_component(char* path);
    void snapshot(const char* filename);
    bool compare(char* filename);
    void dir_saver(const char* path, int index, const char* vulnerable_dir);
    int open_file(const char *filename);
    void write_data(int fd, const char *data);
    void close_file(int fd);
    void write_file_info(int fd, const struct stat *fileStat, const char *path);
    void create_file(char *dirname,int index);
    void print(int index);
    void add_all_dirs(char** argv, int argc);
    void get_file_info(const char *path, struct stat *fileStat);
    bool check_if_mal(const char* path);
    int* count_lines_words_chars(const char* path);
    
int* count_lines_words_chars(const char* filepath) {
    int pipe_fd[2];
    pid_t pid;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return NULL;
    }
   
    //fork to create child process for the script
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return NULL;
    }

    if (pid == 0) { //child process
        close(pipe_fd[0]);
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return NULL;
        }
        close(pipe_fd[1]);
        execlp("bash", "bash", "./count_lines_words_chars.sh", filepath, NULL);
        perror("execlp");
        return NULL;
    } else { //parent process
        close(pipe_fd[1]);
        wait(NULL);
        char buffer[100];
        ssize_t bytes_read = read(pipe_fd[0], buffer, 99);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            int lines, words, characters;
            if (sscanf(buffer, "%d %d %d", &lines, &words, &characters) != 3) {
                return NULL;
            } else {
                int *counts = (int *)malloc(3 * sizeof(int));
                if (counts == NULL) {
                    perror("malloc");
                    return NULL;
                }
                counts[0] = lines;  
                counts[1] = words;
                counts[2] = characters;
                return counts;
            }
        }
    }
}

char* get_last_component(char* path) {
    char *last_component;
    last_component = strrchr(path, '/');
    if (last_component != NULL) {
        last_component++; 
    } else {
        last_component = path;
    }
    return last_component;
}

void replace_char(char *str, char old_char, char new_char) {
    while (*str) {
        if (*str == old_char) {
            *str = new_char;
        }
        str++;
    }
}

void snapshot(const char *filename) {
    int file_descriptor = open_file("snapshot.txt");
    struct stat file_stat;
    get_file_info(filename, &file_stat); 
    write_file_info(file_descriptor, &file_stat, filename);
    close_file(file_descriptor);
}

bool compare(char* filename) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return false;
    } else if (pid == 0) { //child process
        execlp("bash", "bash", "./comparison.sh", "snapshot.txt", filename, NULL);
        perror("execlp");
        exit(1);
    } else { //parent process
        int status;
        waitpid(pid, &status, 0);

        //print the result based on exit status
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                return false; //no difference found
            } else {
                return true; //difference found
            }
        } else {
            printf("Child process did not terminate normally\n");
            return false;
        }
    }
}

void dir_saver(const char* path, int index, const char* vulnerable_dir) {
    DIR* dir;
    struct dirent* entry;
    struct stat info;

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        exit(1);
    }

    dirs* current = root[index];
    while (current != NULL && current->next != NULL) {
        current = current->next;
    }

    if (root[index] == NULL) {
        root[index] = (dirs*)malloc(sizeof(dirs));
        root[index]->next = NULL;
        root[index]->file_head = NULL;
        strcpy(root[index]->name, path);
        strcpy(root[index]->path, path);
        current = root[index];
    } else {
        current->next = (dirs*)malloc(sizeof(dirs));
        current->next->next = NULL;
        current->next->file_head = NULL;
        strcpy(current->next->name, get_last_component(path));
        strcpy(current->next->path, path);
        current = current->next;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
            continue; // Ignore '.' and '..'
        }

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);

        if (stat(full_path, &info) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            dir_saver(full_path, index, vulnerable_dir); // Recursively process directories
        } else {
            char filename[PATH_MAX];
            snprintf(filename, PATH_MAX, "%s/%s", current->path, entry->d_name);

            struct stat file_stat;
            get_file_info(filename, &file_stat);
            bool is_malicious = false;

            if ((file_stat.st_mode & 0777) != 0000) {
                int* wcl = count_lines_words_chars(filename);
                if (wcl[0] <= 3 && wcl[1] >= 1000 && wcl[2] >= 2000) {
                    is_malicious = check_if_mal(filename);
                }
                free(wcl);
            } else if ((file_stat.st_mode & 0777) == 0000) {
                is_malicious = check_if_mal(filename);
            }

            if (is_malicious) {
                cnt++;
                char destination[PATH_MAX];
                char buffer[100];
                strcpy(buffer, filename);
                replace_char(buffer, '/', '-');
                snprintf(destination, PATH_MAX, "%s/%s", vulnerable_dir, buffer);
                if (rename(filename, destination) != 0) {
                    perror("rename");
                    exit(1);
                }
            } else {
                files* fcurrent = current->file_head;
                if (fcurrent == NULL) {
                    fcurrent = (files*)malloc(sizeof(files));
                    fcurrent->next = NULL;
                    strcpy(fcurrent->name, entry->d_name);
                    current->file_head = fcurrent;
                } else {
                    while (fcurrent->next != NULL)
                        fcurrent = fcurrent->next;
                    fcurrent->next = (files*)malloc(sizeof(files));
                    fcurrent->next->next = NULL;
                    strcpy(fcurrent->next->name, entry->d_name);
                }
            }
        }
    }

    closedir(dir);
}

bool check_if_mal(const char* path) {
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return false;
    } else if (pid == 0) { // Child process
        close(pipe_fd[0]);
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return false;
        }
        close(pipe_fd[1]);

        execlp("bash", "bash", "./search_mal.sh", path, NULL);
        perror("execlp");
        return false;
    } else { // Parent process
        wait(NULL);
        close(pipe_fd[1]);
        char buffer[100];
        ssize_t bytes_read = read(pipe_fd[0], buffer, 99);
        if (bytes_read <= 0) {
            perror("read");
            close(pipe_fd[0]);
            return false;
        }
        close(pipe_fd[0]);
        buffer[bytes_read] = '\0';
        if (strstr(buffer, "malicious")) {
            return true;
        } else {
            return false;
        }
    }
}

int open_file(const char *filename) {
    int file_descriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_descriptor == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    return file_descriptor;
}

void write_data(int fd, const char *data) {
    ssize_t bytes_written = write(fd, data, strlen(data));
    if (bytes_written == -1) {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

void close_file(int file_descriptor) {
    if (close(file_descriptor) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }
}

void get_file_info(const char *path, struct stat *file_stat) {
    if (stat(path, file_stat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }
}

void write_file_info(int file_descriptor, const struct stat *file_stat, const char *path) {
    char buffer[1024]; // Buffer to hold the formatted data

    // Write directory path or file name to file
    if (S_ISDIR(file_stat->st_mode)) {
        snprintf(buffer, sizeof(buffer), "Directory: %s\n", path);
    } else {
        snprintf(buffer, sizeof(buffer), "File: %s\n", path);
    }
    write_data(file_descriptor, buffer);

    // Write file size to file
    snprintf(buffer, sizeof(buffer), "File size: %ld bytes\n", file_stat->st_size);
    write_data(file_descriptor, buffer);

    // Write owner UID to file
    snprintf(buffer, sizeof(buffer), "Owner UID: %d\n", file_stat->st_uid);
    write_data(file_descriptor, buffer);

    // Write group ID to file
    snprintf(buffer, sizeof(buffer), "Group ID: %d\n", file_stat->st_gid);
    write_data(file_descriptor, buffer);

    // Write file permissions to file
    snprintf(buffer, sizeof(buffer), "File permissions: %o\n", file_stat->st_mode & 0777);
    write_data(file_descriptor, buffer);
}

void create_file(char *directory_name, int index) {
    dirs* current = root[index];

    while (current != NULL) {
        char filename[PATH_MAX];

        char* s = (char*) malloc(strlen(current->path) + 1);
        strcpy(s, current->path);
        replace_char(s, '/', '-');

        snprintf(filename, PATH_MAX, "%s/%s_data.txt", directory_name, s);

        bool is_file_malicious = true;
        if (access(filename, F_OK) == 0) {
            snapshot(current->path);
            is_file_malicious = compare(filename);
        }

        if (is_file_malicious) {
            int fd = open_file(filename);
            struct stat file_stat;
            get_file_info(current->path, &file_stat);
            write_file_info(fd, &file_stat, current->path);
            close_file(fd);
        }

        files* fcurrent = current->file_head;
        while (fcurrent != NULL) {
            snprintf(filename, PATH_MAX, "%s/%s-%s_data.txt", directory_name, s, fcurrent->name);

            char file_path[PATH_MAX];
            snprintf(file_path, PATH_MAX, "%s/%s", current->path, fcurrent->name);

            bool is_file_malicious = true;
            if (access(filename, F_OK) == 0) {
                snapshot(file_path);
                is_file_malicious = compare(filename);
            }   

            if (is_file_malicious) {
                int fd = open_file(filename);
                struct stat file_stat;
                get_file_info(current->path, &file_stat);
                write_file_info(fd, &file_stat, file_path);
                close_file(fd);
            }

            fcurrent = fcurrent->next;
        }

        free(s);
        current = current->next;
    }
}

void print(int index) {
    dirs* current_directory = root[index];
    while (current_directory != NULL) {
        printf("Directory: %s\n", current_directory->path);
        files* current_file = current_directory->file_head;
        while (current_file != NULL) {
            printf("File: %s/%s\n", current_directory->path, current_file->name);
            current_file = current_file->next;
        }
        current_directory = current_directory->next;
    }
}

void add_all_dirs(char** arguments, int argument_count) {
    if (argument_count < 4 || strcmp(arguments[1], "-o") != 0 || strcmp(arguments[3], "-m") != 0) {
        fprintf(stderr, "Usage: %s -o <output_directory> -m <vulnerable_dir> <directory1> <directory2> ... <directoryN>\n", arguments[0]);
        exit(EXIT_FAILURE);
    }

    if (argument_count > 15) {
        fprintf(stderr, "Too many arguments, there should be -o output_file -m vulnerable_dir and a maximum of 10 directories.\n");
        exit(EXIT_FAILURE);
    }

    struct stat path_stat;
    if (stat(arguments[2], &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Error: Output directory %s is not valid or could not be accessed.\n", arguments[2]);
        exit(EXIT_FAILURE);
    }

    if (stat(arguments[4], &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Error: Vulnerable directory %s is not valid or could not be accessed.\n", arguments[4]);
        exit(EXIT_FAILURE);
    }

    int *malicious_count;
    int index = 0;
    malicious_count = mmap(NULL, (argument_count - 5) * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (malicious_count == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    pid_t child_pids[argument_count - 5];
    for (int i = 5; i < argument_count; i++) {
        if (stat(arguments[i], &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
            fprintf(stderr, "Error: Directory %s is not valid or could not be accessed.\n", arguments[i]);
        } else {
            pid_t pid = fork();

            if (pid == -1) { // Error in child process creation
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Child process
                printf("Snapshot for Directory %d created successfully.\n", index);
                cnt = 0;
                dir_saver(arguments[i], index, arguments[4]);
                malicious_count[index] = cnt;
                create_file(arguments[2], index);
                exit(EXIT_SUCCESS);
            } else { // Parent process
                child_pids[index] = pid;
                index++;
            }
        }
    }

    for (int i = 0; i < index; i++) {
        int status;
        pid_t pid = waitpid(child_pids[i], &status, 0);
        if (pid == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        } else {
            printf("Child Process %d terminated with PID %d and vulnerable files %d.\n", i , pid, malicious_count[i]);
        }
    }
}

int main(int argc, char* argv[]){
    add_all_dirs(argv,argc);

    return 0;
}
