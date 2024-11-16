#include <stdio.h>
#include <string.h>
#include <locale.h>

int main() {
    setlocale(LC_ALL, "");
    char a[1024];

    while (1) {
        printf("$ ");
        if (fgets(a, sizeof(a), stdin) == NULL) {
            printf("Завершение работы\n"); // CTRL + D            
            break;
        }

        size_t len = strlen(a);
        if (len > 0 && a[len - 1] == '\n') {
            a[len - 1] = '\0';
        }

        if (strcmp(a, "exit") == 0 || strcmp(a, "\\q") == 0) { //exit or \q
            printf("Выход\n");
            break;
        } else if (strncmp(a, "echo ", 5) == 0) { //echo
            printf("%s\n", a + 5);
        } else {
            printf("Неизвестная команда\n");
        }
    }

    return 0;
}
