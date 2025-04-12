#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_PATH 512

int isValidFileName(const char *filename) {
    char name[256];
    strncpy(name, filename, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    char *dot = strchr(name, '.');
    if (dot) *dot = '\0';

    return strlen(name) == 1 && isalnum(name[0]);
}

void filterFiles(const char *sourceFolder) {
    DIR *dir = opendir(sourceFolder);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct stat st = {0};
    if (stat("Filtered", &st) == -1 && mkdir("Filtered", 0777) == -1) {
        perror("mkdir");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", sourceFolder, entry->d_name);

        if (entry->d_type == DT_DIR) {
            filterFiles(path);
        } else if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            if (isValidFileName(entry->d_name)) {
                char dest[MAX_PATH];
                snprintf(dest, sizeof(dest), "Filtered/%s", entry->d_name);
                if (rename(path, dest) != 0) perror("rename");
            } else {
                if (remove(path) != 0) perror("remove");
            }
        }
    }

    closedir(dir);
}
// soal a
void setupClues() {
    
    struct stat st = {0};
    if (stat("Clues", &st) == -1) {

        pid_t child1 = fork();
        if (child1 == 0) {
            execlp("curl", "curl", "-L", "-o", "Clues.zip",
                   "https://drive.usercontent.google.com/u/0/uc?id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK&export=download",
                   NULL);
            exit(EXIT_FAILURE);
        }
        wait(NULL);

        pid_t child2 = fork();
        if (child2 == 0) {
            execlp("unzip", "unzip", "-o", "Clues.zip", NULL); 
            exit(EXIT_FAILURE);
        }
        wait(NULL);

        pid_t child3 = fork();
        if (child3 == 0) {
            execlp("rm", "rm", "Clues.zip", NULL);
            exit(EXIT_FAILURE);
        }
        wait(NULL);
    }
}

int cmpstr(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}

//soal c
void combineFiles(const char *folderPath) {
    DIR *dir = opendir(folderPath);
    if (!dir) {
        perror("Tidak bisa membuka folder");
        return;
    }

    char *angka[100], *huruf[100];
    int na = 0, nh = 0;

    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_type == DT_REG && strstr(e->d_name, ".txt")) {
            if (isdigit(e->d_name[0])) angka[na++] = strdup(e->d_name);
            else if (isalpha(e->d_name[0])) huruf[nh++] = strdup(e->d_name);
        }
    }
    closedir(dir);

    qsort(angka, na, sizeof(char *), cmpstr);
    qsort(huruf, nh, sizeof(char *), cmpstr);

    printf("Urutan penggabungan file :\n");
    for (int i = 0; i < na || i < nh; i++) {
        if (i < na) printf("%s\n", angka[i]);
        if (i < nh) printf("%s\n", huruf[i]);
    }

    FILE *out = fopen("Combined.txt", "w");
    if (!out) {
        perror("Gagal membuka Combined.txt");
        return;
    }

    // Gabung file sesuai urutan angka-huruf-angka
    for (int i = 0; i < na || i < nh; i++) {
        char *files[2] = { i < na ? angka[i] : NULL, i < nh ? huruf[i] : NULL };

        for (int j = 0; j < 2; j++) {
            if (!files[j]) continue;

            char path[512], buffer[1024];
            snprintf(path, sizeof(path), "%s/%s", folderPath, files[j]);
            FILE *f = fopen(path, "r");

            if (f) {
                while (fgets(buffer, sizeof(buffer), f))
                    fputs(buffer, out);
                fclose(f);
                remove(path);
            }

            free(files[j]);
        }
    }

    fclose(out);
    puts("Semua file berhasil digabung ke Combined.txt");
}

// soal decoding Rot13
void decodeRot13(const char *inputFile, const char *outputFile) {
    FILE *input = fopen(inputFile, "r");
    if (!input) {
        perror("Gagal membuka Combined.txt untuk membaca");
        return;
    }

    FILE *output = fopen(outputFile, "w");
    if (!output) {
        perror("Gagal membuka Decoded.txt untuk menulis");
        fclose(input);
        return;
    }

    char ch;
    while ((ch = fgetc(input)) != EOF) {
        if ((ch >= 'a' && ch <= 'z')) {
            ch = ((ch - 'a' + 13) % 26) + 'a';
        } else if ((ch >= 'A' && ch <= 'Z')) {
            ch = ((ch - 'A' + 13) % 26) + 'A';
        }
        fputc(ch, output);
    }

    fclose(input);
    fclose(output);
    printf("File berhasil didecode dan disimpan di Decoded.txt\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        setupClues();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filterFiles("Clues");
            printf(" Filtering selesai. File valid telah dipindahkan ke folder Filtered.\n");
            return 0;
        } else if (strcmp(argv[2], "Combine") == 0) {
            combineFiles("Filtered");
            return 0;
        } else if (strcmp(argv[2], "Decode") == 0) {
            decodeRot13("Combined.txt", "Decoded.txt");
            return 0;
        } else {
            fprintf(stderr, "Error.\n");
        }
    } else {
        fprintf(stderr, " Error: Invalid Argument.\n");
    }

    //Error Handling
    printf("Please Use:\n");
    printf("  ./program\n");
    printf("  ./program -m Filter\n");
    printf("  ./program -m Combine\n");
    printf("  ./program -m Decode\n");
    return 1;
}