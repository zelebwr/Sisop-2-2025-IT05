#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

void write_log(const char *process_name, const char *status) {
    FILE *log = fopen("debugmon.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "[%d:%m:%Y]-[%H:%M:%S]", t);

    fprintf(log, "%s_%s_STATUS(%s)\n", timestamp, process_name, status);
    fclose(log);
}

void printDebugmonArt()
{
    printf("==== ⚡️DEBUGMON ⚡️====\n");
    printf("      (⌐■_■)\n");
    printf("   ⌒(￣▽￣)⌒\n");
    printf("    /|   |\\  \n");
    printf("   /_|___|_\\ \n");
    printf("     /   \\ \n");
}

void a_processes(const char *user)
{
    printf("List proses untuk user '%s':\n", user);
    printf("PID    ||\t   Command\t  || CPU || Memory\n");
    printf("---------------------------------------------\n");
    
    char command[100];
    sprintf(command, "ps -u %s -o pid,cmd,%%cpu,%%mem --no-headers", user);
    system(command);
}

void b_start(const char *user)
{
    printf("\nMemulai daemon untuk memantau user '%s'...\n", user);
    write_log("START_DAEMON", "RUNNING");
    
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        write_log("DAEMON_FAILED", "FAILED");
        return;
    }
    
    if (pid > 0)
    {
        printf("Daemon berjalan (PID: %d)\n", pid);
        return;
    }
    
    setsid();
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    while (1)
    {
        char ps_command[100];
        sprintf(ps_command, "ps -u %s -o cmd --no-headers", user);
        FILE *fp = popen(ps_command, "r");
        
        if (fp)
        {
            char process_name[256];
            while (fgets(process_name, sizeof(process_name), fp))
            {
                process_name[strcspn(process_name, "\n")] = '\0';
                write_log(process_name, "RUNNING");
            }
            pclose(fp);
        }
        sleep(5);
    }
}

void c_stop(const char *user) {
    printf("\n>>Menghentikan pengawasan untuk user '%s'...\n", user);

    char cmd[256];
    sprintf(cmd, "pgrep -f 'debugmon daemon %s'", user);
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char pid_str[16];
        if (fgets(pid_str, sizeof(pid_str), fp)) {
            pid_t pid = atoi(pid_str);
            if (kill(pid, SIGTERM) == 0) {
                printf("Yay! Daemon (PID: %d) berhasil dihentikan.\n", pid);
            } else {
                perror("Oh No! Gagal menghentikan daemon");
            }
        } else {
            printf("Oh No! Tidak ada daemon yang aktif untuk user '%s'.\n", user);
        }
        pclose(fp);
    }
}

void d_fail(const char *user) {
    printf("\n>>Menggagalkan SEMUA proses user '%s'...\n", user);
    write_log("FAIL_ALL_PROCESSES", "STARTED");

    char ps_command[100];
    sprintf(ps_command, "ps -u %s -o pid,cmd --no-headers", user);
    FILE *fp = popen(ps_command, "r");
    
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char *cmd = strchr(line, ' ') + 1;
            cmd[strcspn(cmd, "\n")] = '\0';
            write_log(cmd, "FAILED");
        }
        pclose(fp);
    }

    char kill_command[100];
    sprintf(kill_command, "pkill -9 -u %s", user);
    int result = system(kill_command);

    if (result == 0)
    {
        printf("Yay! Semua proses user '%s' berhasil digagalkan\n", user);
        write_log("FAIL_ALL_PROCESSES", "COMPLETED");
    } 
    else
    {
        printf("Oh No! Gagal menghentikan proses user '%s'\n", user);
        write_log("FAIL_ALL_PROCESSES", "FAILED");
    }

    sprintf(kill_command, "sudo usermod -L %s", user);
    system(kill_command);
    printf("Sorry! User '%s' dikunci dan tidak bisa menjalankan proses baru\n", user);
}

void e_revert(const char *user) {
    printf("\n>> Memulihkan akses user '%s'...\n", user);
    write_log("REVERT_USER", "STARTED");

    char revert_command[100];
    sprintf(revert_command, "sudo usermod -U %s", user);
    int result = system(revert_command);

    if (result == 0) {
        printf("Yay! Akses user '%s' berhasil dipulihkan\n", user);
        write_log("REVERT_USER", "COMPLETED");
    } else {
        printf("Oh No! Gagal memulihkan akses user '%s'\n", user);
        write_log("REVERT_USER", "FAILED");
    }
}

int main() {
    char input[100];
    char cmd[20];
    char user[50];

    do {
        printDebugmonArt();
        printf("List of Commands:\n");
        printf("1. list <user>\n");
        printf("2. daemon <user>\n");
        printf("3. stop <user>\n"); 
        printf("4. fail <user>\n");
        printf("5. revert <user>\n");
        printf("6. exit\n");
        printf("\n>> ");
        
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        
        if (sscanf(input, "%s %s", cmd, user) < 1) {
            printf("❌ Format command tidak valid\n");
            continue;
        }

        if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else if (strcmp(cmd, "list") == 0 && strlen(user) > 0) {
            a_processes(user);
        }
        else if (strcmp(cmd, "daemon") == 0 && strlen(user) > 0) {
            b_start(user);
        }
        else if (strcmp(cmd, "stop") == 0 && strlen(user) > 0) {
            c_stop(user);
        }
        else if (strcmp(cmd, "fail") == 0 && strlen(user) > 0) {
            d_fail(user);
        }
        else if (strcmp(cmd, "revert") == 0 && strlen(user) > 0) {
            e_revert(user);
        }
        else {
            printf("❌ Command tidak valid atau username kosong\n");
        }
    } while (1);
    
    printf("Keluar dari program...\n");
    return 0;
}