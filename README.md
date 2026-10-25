# VCSRelink

Статический рекомпилятор ROM-ов Atari 2600 в нативный C++ для Linux — без эмуляции, настоящая бинарная трансляция.

## Как это работает

1. `src/relinker.cpp` читает образ ROM (4K, либо 8K с переключением банков F8), дизассемблирует его и генерирует `build/relinked_game.cpp`: нативную C++-функцию на каждую инструкцию 6502 и таблицу переходов между ними.
2. `src/runtime.cpp` — рантайм: TIA (видео: playfield, спрайты, мяч, HMOVE, приоритеты, звук), RIOT (128 байт RAM, таймеры, порты), джойстик и консольные кнопки, вывод через SDL2 (160x200, PAL 50 Гц).
3. `src/main.cpp` — точка входа: `rt::init()` и `relink_cpu_exec()`.

## Требования

- Linux, g++ с поддержкой C++17, make, SDL2 (`libsdl2-dev`).
- Образ ROM Atari 2600 (4K или 8K F8) — положить в `Rom/*.bin`. ROM-ы в репозиторий не входят.

## Сборка

```sh
make        # собрать build/relinker, сгенерировать код игры и собрать ./vcsrelink
make run    # собрать и запустить
make gen    # только сгенерировать build/relinked_game.cpp
make clean  # удалить build/ и бинарник
```

## Запуск

```sh
./vcsrelink [--scale N] [--headless] [--frames N] [--ppm FILE] [--nopace]
```

| Параметр | Назначение |
| --- | --- |
| `--scale N` | масштаб окна (по умолчанию 2) |
| `--headless` | без окна и звука |
| `--frames N` | остановиться после N кадров |
| `--ppm FILE` | сохранить последний кадр в PPM |
| `--nopace` | не ограничивать частоту 50 Гц |

## Управление

| Клавиши | Действие |
| --- | --- |
| Стрелки / WASD | Джойстик (движение) |
| Space / Z | Кнопка Fire |
| Tab | Select |
| Enter / Backspace | Reset |
| F1 | Справка по клавишам |
| F2 | Консоль состояния TIA/RIOT/CPU |
| P | Пауза |
| Esc | Выход |

Reset нажимается автоматически на ~30-м кадре, чтобы игра стартовала без ручного вмешательства. Отключается через `VCS_NOAUTOSTART=1` или явным `VCS_RESETFRAME`.

## Отладка

Переменные окружения:

- `VCS_INPUT=right,left,up,down,fire,select,reset` — удерживать кнопки (удобно для headless-прогонов).
- `VCS_FRAMESTATS=1` — статистика по кадрам: число строк, WSYNC, таймеры, тики.
- `VCS_RAMTRACE=1` — изменения ключевых ячеек RAM по кадрам.
- `VCS_LOG_FRAME=N` — дамп регистров CPU/TIA на кадре N.
- `VCS_TRACE_FRAME=N`, `VCS_TRACE_ALL=<limit>` — трассировка исполняемых инструкций.
- `VCS_DISPLAY_PPM=FILE` + `VCS_DISPLAY_FRAME=N` — сохранить кадр N в PPM (по умолчанию кадр 5).
- `VCS_WRTRACE`, `VCS_RDTRACE`, `VCS_IODBG`, `VCS_COLLDBG`, `VCS_POSDBG`, `VCS_REGTRACE`, `VCS_RAMDUMP=N` — точечные трассы записи/чтения, коллизий, позиций спрайтов, регистров и дамп RAM.
- `VCS_NOPACE`, `VCS_NOVSYNC` — отключить ограничение частоты кадров / вертикальную синхронизацию.

## Структура

```
.
├── Makefile
├── Rom/                  # ваши ROM-ы (*.bin), в git не попадают
├── src/
│   ├── relinker.cpp      # 6502 -> C++ рекомпилятор
│   ├── runtime.hpp       # API рантайма для сгенерированного кода
│   ├── runtime.cpp       # TIA/RIOT/SDL2/ввод/отладка
│   ├── font5x7.hpp       # шрифт встроенной консоли
│   └── main.cpp
└── build/                # сгенерированный код и объектные файлы
```
