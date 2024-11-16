#!/bin/sh

set -e # Завершаем выполнение при ошибке

# Компиляция программы в объектный файл (не создаём исполнимый файл)
gcc -c app/*.c -o /tmp/shell.o

# Линковка объектных файлов в исполнимый файл в /tmp (можно использовать временную директорию)
gcc /tmp/shell.o -o /tmp/shell-target

# Запуск программы
/tmp/shell-target "$@"

# Удаление исполнимого файла после выполнения
rm -f /tmp/shell-target /tmp/shell.o