// starterkit.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#define SIZE 4096
pid_t decrypt_pid = -1;  //deklarasi awal pidnya blm ada proses

//membuat activity.log
void Log(const char* message) {
    FILE *logFile = fopen("activity.log", "a"); //buat file dgn mode append
    if (logFile == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(logFile, "[%02d-%02d-%d][%02d:%02d:%02d] - %s\n",
        t->tm_mday, t->tm_mon+1, t->tm_year+1900,
        t->tm_hour, t->tm_min, t->tm_sec,
        message);

    fclose(logFile);
}
//check apakah file base64
int is_base64(const char *str) {
    int len = strlen(str); 
    if (len % 4 != 0) return 0; 

    for (int i = 0; i < len; i++) {
        char c = str[i];
        
        if (!((c >= 'A' && c <= 'Z') || 
              (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || 
              c == '+' ||               
              c == '/' ||               
              c == '='))                
            return 0;
    }
    return 1; 
}

//soal a
void setup() {
    pid_t child1, child2, child3;

    mkdir("starter_kit", 0777);

    child1 = fork();
    if (child1 == 0) {
        execlp("curl", "curl", "-L", "-o", "starter_kit.zip",
               "https://drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download", NULL);
        exit(EXIT_FAILURE);
    }
    wait(NULL);

    child2 = fork();
    if (child2 == 0) {
        execlp("unzip", "unzip", "-o", "starter_kit.zip", "-d", "starter_kit", NULL);
        exit(EXIT_FAILURE);
    }
    wait(NULL);

    child3 = fork();
    if (child3 == 0) {
        execlp("rm", "rm", "starter_kit.zip", NULL);
        exit(EXIT_FAILURE);
    }
    wait(NULL);
}

//soal b
void decrypt() {
    pid_t pid, sid;
    pid = fork();

    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        FILE *pidFile = fopen("decrypt.pid", "w");
        if (pidFile != NULL) {
            fprintf(pidFile, "%d", pid);
            fclose(pidFile);
        }
        exit(EXIT_SUCCESS);
    }

    umask(0);
    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);
    if ((chdir(".")) < 0) exit(EXIT_FAILURE);

    mkdir("quarantine", 0777);

    while (1) {
        DIR *dir;
        struct dirent *entry;

        // Cek di starter_kit → pindah & decrypt
        dir = opendir("starter_kit");
        if (dir != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG && is_base64(entry->d_name)) {
                    char src_path[SIZE], dst_path[SIZE], decoded[SIZE];

                    snprintf(src_path, sizeof(src_path), "starter_kit/%s", entry->d_name);

                    // Decode base64
                    FILE *fp;
                    char cmd[SIZE];
                    snprintf(cmd, sizeof(cmd), "echo %s | base64 -d", entry->d_name);
                    fp = popen(cmd, "r");
                    if (fp == NULL) continue;
                    fgets(decoded, sizeof(decoded), fp);
                    decoded[strcspn(decoded, "\n")] = '\0';
                    pclose(fp);

                    // Pindah dan rename langsung ke quarantine
                    snprintf(dst_path, sizeof(dst_path), "quarantine/%s", decoded);
                    rename(src_path, dst_path);
                }
            }
            closedir(dir);
        }

        //Cek di quarantine → decrypt jika masih base64
        dir = opendir("quarantine");
        if (dir != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG && is_base64(entry->d_name)) {
                    char oldpath[SIZE], newname[SIZE], newpath[SIZE];

                    snprintf(oldpath, sizeof(oldpath), "quarantine/%s", entry->d_name);

                    // Decode base64
                    FILE *fp;
                    char cmd[SIZE];
                    snprintf(cmd, sizeof(cmd), "echo %s | base64 -d", entry->d_name);
                    fp = popen(cmd, "r");
                    if (fp == NULL) continue;
                    fgets(newname, sizeof(newname), fp);
                    newname[strcspn(newname, "\n")] = '\0';
                    pclose(fp);

                    snprintf(newpath, sizeof(newpath), "quarantine/%s", newname);
                    rename(oldpath, newpath);
                }
            }
            closedir(dir);
        }

        sleep(5);
    }
}


//soal c
void quarantineFiles() {
    DIR *dir = opendir("starter_kit");
    struct dirent *entry;

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char oldpath[SIZE], newpath[SIZE], buffer[SIZE];
            snprintf(oldpath, sizeof(oldpath), "starter_kit/%s", entry->d_name);
            snprintf(newpath, sizeof(newpath), "quarantine/%s", entry->d_name);
            rename(oldpath, newpath);

            snprintf(buffer, sizeof(buffer), "%s - Successfully moved to quarantine directory.", entry->d_name);
            Log(buffer);
        }
    }
    closedir(dir);
}

void returnFiles() {
    DIR *dir = opendir("quarantine");
    struct dirent *entry;

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char oldpath[SIZE], newpath[SIZE], buffer[SIZE];
            snprintf(oldpath, sizeof(oldpath), "quarantine/%s", entry->d_name);
            snprintf(newpath, sizeof(newpath), "starter_kit/%s", entry->d_name);
            rename(oldpath, newpath);

            snprintf(buffer, sizeof(buffer), "%s - Successfully returned to starter kit directory.", entry->d_name);
            Log(buffer);
        }
    }
    closedir(dir);
}

//soal d
void eradicate() {
    DIR *dir = opendir("quarantine");
    struct dirent *entry;

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[SIZE], buffer[SIZE];
            snprintf(filepath, sizeof(filepath), "quarantine/%s", entry->d_name);
            remove(filepath);

            snprintf(buffer, sizeof(buffer), "%s - Successfully deleted.", entry->d_name);
            Log(buffer);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--decrypt") == 0) {
            decrypt_pid = fork();
            if (decrypt_pid == 0) {
                decrypt();
                exit(0);
            } else {
                printf("Decrypt Process Running, PID : %d\n", decrypt_pid);

                char buffer[SIZE];
                snprintf(buffer, sizeof(buffer), "Successfully started decryption process with PID %d.", decrypt_pid);
                Log(buffer);
                
                wait(NULL);
            }
        } else if (strcmp(argv[1], "--shutdown") == 0) {
            FILE *pid_file = fopen("decrypt.pid", "r");
            if (pid_file == NULL) {
                printf("Decrypt daemon not running\n");
                exit(EXIT_FAILURE);
            }
        
            fscanf(pid_file, "%d", &decrypt_pid);
            fclose(pid_file);
        
            if (kill(decrypt_pid, SIGTERM) == 0) {
                printf("Decrypt daemon shutdown success\n");
                char buffer[SIZE];
                snprintf(buffer, sizeof(buffer), "Successfully shut off decryption process with PID %d.", decrypt_pid);
                Log(buffer); // log tambahan untuk shutdown
            
                remove("decrypt.pid"); // hapus file PID
            } else {
                printf("Failed to shutdown decrypt daemon\n");
            }            
        } else if (strcmp(argv[1], "--quarantine") == 0) {
            quarantineFiles();
        } else if (strcmp(argv[1], "--return") == 0) {
            returnFiles();
        } else if (strcmp(argv[1], "--eradicate") == 0) {
            eradicate();
        } else {
            printf("Error.\n");
            printf("Use:\n");
            printf("  --decrypt\n  --quarantine\n  --return\n  --eradicate\n  --shutdown\n");
            exit(EXIT_FAILURE);
        }
    } else {
        setup();
    }

    return 0;
}