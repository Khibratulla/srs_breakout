# Breakout (Arkanoid)

Игра Breakout на C++ с использованием **OpenGL 3.3**, **GLFW** и **GLAD**: шейдеры, VAO/VBO/EBO, uniform-переменные, движение с учётом delta time.

## Управление

| Клавиша | Действие |
|---|---|
| `A` / `←` | Платформа влево |
| `D` / `→` | Платформа вправо |
| `Space` | Запустить мяч |
| `R` | Начать заново |
| `Esc` | Выход |

## Запуск

Запустите `breakout.exe`.

## Сборка (Windows, MinGW-w64)

```bash
g++ main.cpp glad/src/gl.c -Iinclude -Iglad/include -Llib-mingw-w64 -lglfw3 -lgdi32 -o breakout.exe
```

## Структура проекта

```
main.cpp              — исходный код игры
glad/                 — загрузчик OpenGL (gl.h, gl.c, khrplatform.h)
include/GLFW/         — заголовочный файл GLFW
lib-mingw-w64/        — статическая библиотека libglfw3.a
.vscode/              — настройки VS Code
breakout.exe          — собранная программа
```
