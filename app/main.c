#include <stdio.h>
#include <stdlib.h> // Для getenv() и exit()
#include <string.h>
#include <locale.h>
#include <unistd.h> // Для fork() и execl()
#include <sys/wait.h> // Для wait()
#include <signal.h> // Для signal()

#define HISTORY_FILE "history"

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
void handle_sighup(int signal) {
    printf("\nConfiguration reloaded\n");
}

int main() {
    setlocale(LC_ALL, "");
    char a[1024];

    // Устанавливаем обработчик сигнала SIGHUP
    if (signal(SIGHUP, handle_sighup) == SIG_ERR) {
        perror("Ошибка установки обработчика сигнала");
        return 1;
    }

    // Проверяем, существует ли файл истории
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file == NULL) {
        perror("Не удалось открыть или создать файл истории");
        return 1;
    }
    fclose(file);

    printf("Shell PID: %d\n", getpid()); // Выводим PID для тестирования сигналов

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

        // Сохраняем команду в файл истории, кроме пустых строк
        if (strlen(a) > 0) {
            save_to_history(a);
        }

        if (strcmp(a, "exit") == 0 || strcmp(a, "\\q") == 0) {
            printf("Выход\n");
            break;
        } else if (strncmp(a, "echo ", 5) == 0) {
            printf("%s\n", a + 5);
        } else if (strncmp(a, "\\e $", 4) == 0) {
            const char *var_name = a + 4;
            char *value = getenv(var_name);
            if (value) {
                printf("%s\n", value);
            } else {
                printf("Переменная окружения '%s' не найдена\n", var_name);
            }
        } else if (a[0] == '/') {
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
        } else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}
