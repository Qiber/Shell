#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define HISTORY_FILE "history"
#define SECTOR_SIZE 512
#define MBR_SIGNATURE_OFFSET 510

// Функция для сохранения команды в файл истории
void save_to_history(const char *command) {
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file == NULL) {
        perror("Ошибка открытия файла истории");
        return;
    }
    fprintf(file, "%s\n", command);
    fclose(file);
}

// Обработчик сигнала SIGHUP
void handle_sighup(int signum) {
    (void)signum; // Чтобы избежать предупреждения о неиспользуемой переменной
    printf("Configuration reloaded\n");
}

// Функция для проверки загрузочной сигнатуры диска
void check_bootable_disk(const char *disk) {
    char path[256];
    snprintf(path, sizeof(path), "/dev/%s", disk);

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("Ошибка открытия диска");
        return;
    }

    unsigned char buffer[SECTOR_SIZE];
    if (fread(buffer, 1, SECTOR_SIZE, file) != SECTOR_SIZE) {
        perror("Ошибка чтения сектора");
        fclose(file);
        return;
    }
    fclose(file);

    // Проверка сигнатуры в последних двух байтах
    if (buffer[MBR_SIGNATURE_OFFSET] == 0x55 && buffer[MBR_SIGNATURE_OFFSET + 1] == 0xAA) {
        printf("Диск %s является загрузочным (сигнатура 55AA).\n", disk);
    } else {
        printf("Диск %s не является загрузочным.\n", disk);
    }
}

int main() {
    setlocale(LC_ALL, "");

    char a[1024];
    signal(SIGHUP, handle_sighup); // Устанавливаем обработчик сигнала SIGHUP

    // Проверяем или создаём файл истории
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file == NULL) {
        perror("Не удалось открыть или создать файл истории");
        return 1;
    }
    fclose(file);

    while (1) {
        printf("$ ");
        if (fgets(a, sizeof(a), stdin) == NULL) {
            printf("Завершение работы\n");
            break;
        }

        size_t len = strlen(a);
        if (len > 0 && a[len - 1] == '\n') {
            a[len - 1] = '\0';
        }

        if (strlen(a) > 0) {
            save_to_history(a);
        }

        // Команда выхода
        if (strcmp(a, "exit") == 0 || strcmp(a, "\\q") == 0) {
            printf("Выход\n");
            break;
        }
        // Команда echo
        else if (strncmp(a, "echo ", 5) == 0) {
            printf("%s\n", a + 5);
        }
        // Вывод переменной окружения
        else if (strncmp(a, "\\e $", 4) == 0) {
            const char *var_name = a + 4;
            char *value = getenv(var_name);
            if (value) {
                printf("%s\n", value);
            } else {
                printf("Переменная окружения '%s' не найдена\n", var_name);
            }
        }
        // Выполнение бинарника
        else if (a[0] == '/') {
            pid_t pid = fork();
            if (pid == 0) {
                execl(a, a, NULL);
                perror("Ошибка выполнения");
                exit(1);
            } else if (pid > 0) {
                wait(NULL);
            } else {
                perror("Ошибка fork");
            }
        }
        // Проверка загрузочной сигнатуры
        else if (strncmp(a, "\\l ", 3) == 0) {
            check_bootable_disk(a + 3);
        }
        // Неизвестная команда
        else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}
