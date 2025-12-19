# Содержание лабораторных работ

**1.** [Лабораторная работа №1: Создание процессов](#лабораторная-работа-1-создание-процессов)  
**2.** [Лабораторная работа №2: Создание потоков](#лабораторная-работа-2-создание-потоков)  
**3.** [Лабораторная работа №3: Синхронизация потоков](#лабораторная-работа-3-синхронизация-потоков-при-помощи-критических-секций-и-событий-обработка-тупиков)  
**4.** [Лабораторная работа №4: Синхронизация процессов](#лабораторная-работа-4-синхронизация-процессов)
**5.** [Лабораторная работа №5: Обмен данными по именованным каналам](#лабораторная-работа-5-обмен-данными-по-именованным-каналам)  

<details>
<summary> Подробное описание сборки</summary>
  
| Технология | Назначение | Версия |
|------------|------------|---------|
| **C++** | Основной язык разработки | C++20 |
| **WinAPI** | Работа с потоками и синхронизацией | - |
| **Google Test**; **DocTest**; **Catch2** | Юнит-тестирование | Последняя |
| **CMake** | Система сборки | 3.8+ |

</details>

---

# Лабораторная работа №1: "Создание процессов"

## Задача

Написать консольную программу Main и две утилиты (служебные программы) Creator и Reporter, которые выполняют требуемые действия.

<details>
<summary>Архитектура</summary>

1. **main.exe**
   - Управляет процессами и запрашивает данные у пользователя.

2. **Creator.exe**
   - Создает бинарный файл для хранения данных.

3. **Reporter.exe**
   - Обрабатывает данные из бинарного файла и формирует по ним отчёт.

</details>

## Структура проекта

```
Lab_1/
├── 📄 main                  //Папка с main.cpp и Header.h
├── 📄 Creator               //Папка с Creator.cpp
├── 📄 Reporter              //Папка с Reporter.cpp
├── 🧪 For_tests             //Папка с тестируемыми блоками
├── 🧪 tests                 //Юнит-тесты Google Test
├── ⚙️ CMakeLists.txt        //Конфигурация сборки
```


## Пример работы

# Запуск программы
> main.exe

Enter the name of the binary file: employees
Enter the number of employees: 2

┌─────────────────────────────────┐
│  Запуск Creator.exe...          │
└─────────────────────────────────┘

Employee 1
Number: 101
Name: Ivanov
Hours: 160

Employee 2:
Number: 102
Name: Petrov
Hours: 145.5

Enter the name of the report file: otchet
Enter the payment for the hour of work: 15.5

┌─────────────────────────────────┐
│  Работа Reporter.exe...         │
└─────────────────────────────────┘

Report Content:
101 Ivanov 160.00 2480.00
102 Petrov 145.50 2255.25

```

</details>

---

# Лабораторная работа №2: "Создание потоков"

## Задача

Написать программу для консольного процесса, который состоит из трех потоков: main, min_max и average.

<details>
<summary>Архитектура</summary>

1. **main.exe**
   - Создаёт потоки, которые работаю с массивом.

2. **main_2.exe**
   - Аналогично на <thread>

</details>

## Структура проекта

```
lab_2/
├── 📄 main              // Папка с main.cpp и Header.h
├── 📄 main_2            // Папка с main_thread.cpp и Header.h
├── 🧪 Tests             // Юнит-тесты DocTest
├── ⚙️ CMakeLists.txt    // Конфигурация сборки
```

## Пример работы

# Запуск программы
> main.exe

Enter array size: 5
Enter 5 elements: 10 20 30 40 50

┌─────────────────────────────────┐
│  Работа потоков...              │
└─────────────────────────────────┘

Final array: 30 20 30 40 30
```

</details>

---

# Лабораторная работа №3: "Синхронизация потоков при помощи критических секций и событий. Обработка тупиков"

## Задача
Использование мьютексов и ивентов.
Написать программу для консольного процесса, который состоит из потока main и нескольких экземпляров потока marker.

<details>
<summary>Архитектура</summary>

1. **main.exe**
   - Управляет n-нным количеством потоков, которые работают с массимов.

</details>

## Структура проекта

```
Lab_3/
├── 📄 main                // Папка с main.cpp
├── 🧪 tests.cpp           // Юнит-тесты Catch2, tread.cpp и Header.cpp с тестируемыми блоками
├── ⚙️ CMakeLists.txt      // Конфигурация сборки
```

## Пример работы

# Запуск программы
> main.exe

Enter the size of the array: 10
Enter the number of threads: 3

┌─────────────────────────────────┐
│  Запуск потоков marker...       │
└─────────────────────────────────┘

Thread number: 1
Number of marked elements: 5
It is not possible to mark an element with an index: 3
Thread number: 2
Number of marked elements: 4
It is not possible to mark an element with an index: 7
Thread number: 3
Number of marked elements: 6
It is not possible to mark an element with an index: 2

┌─────────────────────────────────┐
│  Все потоки ожидают             │
└─────────────────────────────────┘

Array contents: 1 2 3 1 2 0 3 2 1 3

Enter the thread number to complete: 2

Array contents after stopping thread 2:
1 0 3 1 0 0 3 0 1 3

┌─────────────────────────────────┐
│ После завершения всех потоков.  │
└─────────────────────────────────┘

All threads have been completed.
```

</details>

---

# Лабораторная работа №4: "Синхронизация процессов"

## Задача
Использование семафоров.
Написать программу для передачи сообщений между процессами через общий файл. Программа включает один процесс Receiver и несколько процессов Sender. Процессы Sender посылают сообщения процессу Receiver.

<details>
<summary>Архитектура</summary>

1. **Receiver.exe**
   - Создаёт процессы Sender и читает данные из бинарного файла, отправленные ими (кольцевая очередь).

2. **Sender.exe**
   - Записывают сообщения в кольцевую очередь

</details>

## Структура проекта

```
Lab_4/
├── 📄 Receiver          // Папка с Receiver.cpp 
├── 📄 Sender            // Папка с Sender.cpp 
├── 🧪 tests.cpp         // Юнит-тесты DocTest, Tests.cpp и Header.h с тестируемыми блоками
├── ⚙️ CMakeLists.txt    // Конфигурация сборки
```


## Пример работы

# Запуск приемника
> Receiver.exe

Enter filename: messages
Enter number of messages: 5
Enter number of processes: 3

┌─────────────────────────────────┐
│  Ожидание отправителей...       │
└─────────────────────────────────┘
All processes are ready to work.

Enter 1 to continue, 2 to exit: 1
Received line: Hello from sender 1

Enter 1 to continue, 2 to exit: 2
Exiting by user command.

# В консолях отправителей:
> sender.exe 

Enter command:
1 - send a message to file
2 - exit
1

Enter the message (each <= 20 characters). Empty line is an input: Hello from sender 1

Enter command:
1 - send a message to file
2 - exit
2

Exiting by user command.
```

</details>


### Механизмы синхронизации
- **Семафор Start** - для проверки отправителей на готовность к работе.
- **Семафор empty** - считает свободные строки для сообщений.
- **Семафор full** - считает заполненные строки.
- **Семафор mutex** - координация чтения/записи (доступа к файлу).

---
# Лабораторная работа №5: "Обмен данными по именованным каналам"

## Задача
Написать программу для управления доступом параллельных процессов к файлу по именованным каналам.

<details>
<summary>Архитектура</summary>

1. **Server.exe/Server_98.exe/Server_thread.exe**
   - Запускает клиентов, обеспечитают для них доступ к записи в бинарном файле: чтение/модификацию.

2. **Client.exe/Client_98.exe/Client_thread.exe**
   - Обрабатывают запросы клиентов в зависимости от выбора.

</details>

## Структура проекта

```
Lab_5/
├── 📄 Server             // Папка с Server.cpp 
├── 📄 Server_98          // Папка с Server_98.cpp 
├── 📄 Server_thread      // Папка с Server_thread.cpp 
├── 📄 Client             // Папка с Client.cpp
├── 📄 Client_98          // Папка с Client_98.cpp
├── 📄 Client_thread      // Папка с Client_thread.cpp 
├── 🧪 tests.cpp          // Юнит-тесты Catch2, functions.cpp и Header.h с тестируемыми блоками
├── ⚙️ CMakeLists.txt     // Конфигурация сборки
```


## Пример работы

# Запуск сервера
> Server.exe

Enter filename: employee
Enter the number of employees: 1
Enter information about employee number 1:
Employee's id number: 1
Employee's name: 1
Number of working hours: 1
Enter number of processes: 2

┌─────────────────────────────────┐
│  Ожидание отправителей...       │
└─────────────────────────────────┘
Request from client: Reading for employee number 1
Client completed read access to record 1
Request from client: Modification for employee number 1
Record 1 saved. Releasing lock.

File content:
Employee 1
ID: 1
Name: 11
Hours: 11

Enter 1 to shutdown server: 1
Server shutdown complete.

# В консолях отправителей:
> sender.exe 

Operations:
1 - Record modification
2 - Reading a record
3 - Exit
2

Employee's id number: 1

Data about the employee:
ID: 1
Name: 1
Number of working hours: 1

Actions:
1 - Complete access to this record
Select action: 1

Operations:
1 - Record modification
2 - Reading a record
3 - Exit
1

Employee's id number: 1

Data about the employee:
ID: 1
Name: 1
Number of working hours: 1

Enter new name (max 9 chars): 11
Enter new number of working hours: 11

Actions:
1 - Send modified record to server
2 - Complete access to this record (without saving)
Select action: 1

Modification result: SUCCESS
Modification completed. Returning to main menu.

Operations:
1 - Record modification
2 - Reading a record
3 - Exit
3

Completion of work. Exiting.

```

</details>


### Механизмы синхронизации
- SRWLock для блокировки модификации при чтении и блокировки записи при модификации.

---

## Автор

**Студентка 2 курса БГУ ФПМИ**  
📧 Email:
💻 GitHub:

---

<div align="center">

*Лабораторные работа выполнены в рамках курса "Операционные системы"*  
*Белорусский государственный университет, ФПМИ*  
*2025-2026 учебный год*

![БГУ](https://img.shields.io/badge/БГУ-ФПМИ-blue?style=for-the-badge)

</div>

---
