#!/bin/bash
echo "=== Автоматическое развертывание ==="

mkdir -p build
cd build
cmake ..
make -j4
cd ..

echo "Сервер запущен на http://localhost:8080"
echo "Нажмите Ctrl+C для остановки"
./build/todo_app