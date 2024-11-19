#include <stdio.h>
#include <stdlib.h> // Для getenv()
#include <string.h>
#include <locale.h>

#define HISTORY_FILE "history"

// Функция для сохранения команды в файл истории
void save_to_history(const char *command) {
    FILE *file = fopen(HISTORY_FILE, "a"); // Открываем файл для добавления данных
    if (file == NULL) {
        perror("Ошибка открытия файла истории");
        return;
    }
    fprintf(file, "%s\n", command); // Записываем команду в файл
    fclose(file); // Закрываем файл
}

int main() {
    setlocale(LC_ALL, "");
    char a[1024];

    // Проверяем, существует ли файл истории
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file == NULL) {
        perror("Не удалось открыть или создать файл истории");
        return 1;
    }
    fclose(file);

    while (1) {
        printf("$ ");
        if (fgets(a, sizeof(a), stdin) == NULL) {
            printf("Завершение работы\n"); // CTRL + D
            break;
        }

        size_t len = strlen(a);
        if (len > 0 && a[len - 1] == '\n') {
            a[len - 1] = '\0'; // Убираем символ новой строки
        }

        // Сохраняем команду в файл истории, кроме пустых строк
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
        
        // Команда для вывода переменной окружения
        else if (strncmp(a, "\\e $", 4) == 0) {
            const char *var_name = a + 4; // Извлекаем имя переменной (всё после "\e $")
            char *value = getenv(var_name); // Получаем значение переменной
            if (value) {
                printf("%s\n", value); // Вывод значения переменной
            } else {
                printf("Переменная окружения '%s' не найдена\n", var_name);
            }
        } 
        
        // Неизвестная команда
        else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}
