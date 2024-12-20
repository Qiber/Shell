#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>  // Для работы с файловыми дескрипторами
#include <sys/types.h>  // Для типов PID

#define HISTORY_FILE "history"
#define SECTOR_SIZE 512
#define MBR_SIGNATURE_OFFSET 510
#define VFS_PATH "/tmp/vfs"
#define CRON_FILE VFS_PATH "/cron_tasks.txt"

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
    (void)signum;
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

    if (buffer[MBR_SIGNATURE_OFFSET] == 0x55 && buffer[MBR_SIGNATURE_OFFSET + 1] == 0xAA) {
        printf("Диск %s является загрузочным (сигнатура 55AA).\n", disk);
    } else {
        printf("Диск %s не является загрузочным.\n", disk);
    }
}

// Функция для обработки команды \cron
void handle_cron_command() {
    // Создание директории /tmp/vfs
    if (mkdir(VFS_PATH, 0777) == -1 && errno != EEXIST) {
        perror("Ошибка создания VFS директории");
        return;
    }

    // Открытие файла для записи задач cron
    FILE *cron_file = fopen(CRON_FILE, "w");
    if (cron_file == NULL) {
        perror("Ошибка открытия файла cron_tasks.txt");
        return;
    }

    // Запуск команды crontab -l и перенаправление вывода в файл
    FILE *pipe = popen("crontab -l", "r");
    if (pipe == NULL) {
        fprintf(cron_file, "Не удалось получить список задач cron\n");
        fclose(cron_file);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), pipe) != NULL) {
        fputs(line, cron_file);
    }

    pclose(pipe);
    fclose(cron_file);

    printf("VFS создана в %s. Задачи cron сохранены в %s.\n", VFS_PATH, CRON_FILE);
}

// Функция для получения дампа памяти процесса
void create_memory_dump(int pid) {
    // Проверка существования процесса
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
    if (access(proc_path, F_OK) == -1) {
        printf("Процесс с PID %d не существует.\n", pid);
        return;
    }

    // Формируем команду gcore с нужным префиксом для дампа
    char command[256];
    snprintf(command, sizeof(command), "gcore -o core.%d %d", pid, pid);
    
    // Запуск gcore для создания дампа памяти
    int status = system(command);
    if (status == 0) {
        printf("Дамп памяти процесса %d успешно создан.\n", pid);
    } else {
        printf("Ошибка создания дампа памяти для процесса %d.\n", pid);
    }
}

int main() {
    setlocale(LC_ALL, "");

    char a[1024];
    signal(SIGHUP, handle_sighup);

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
        // Команда \cron
        else if (strcmp(a, "\\cron") == 0) {
            handle_cron_command();
        }
        // Получение дампа памяти процесса
        else if (strncmp(a, "\\mem ", 5) == 0) {
            int pid = atoi(a + 5);
            create_memory_dump(pid);
        }
        // Неизвестная команда
        else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}
