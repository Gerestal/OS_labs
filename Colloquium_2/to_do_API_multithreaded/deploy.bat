@echo off
chcp 1251 > nul
echo === Автоматическое развертывание ===

mkdir build 2>nul
cd build
cmake ..
cmake --build . --config Release
cd ..

echo Сервер запущен на http://localhost:8080
echo Нажмите Ctrl+C для остановки
build\Release\todo_app.exe