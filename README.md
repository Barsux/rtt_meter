# rtt_meter - Программа для измерения RTT задержек на L2 уровне, реализованная для Linux.
## Преимущества
	- Микроядерная архитектура
	- Система очередей
	- Простая возможность масшабирования и реализации других измерений
	- Простая возможность внедрить реализацию для другой платформы, не трогая реализацию
## Модули
    - Ядро (core) - является центральной частью программы и реализует в себе связь с другими модулями
    - mgmt - Модуль управления программой, внесения задач, в данной реализации подразумевает внесение команды пользователем и вывод результата.
    - global_setup (Не реализовано) - Модуль для работы с файловой системой
    - Packetizer - Модуль обработки пакетов, связан напрямую с l2_transport модулем
    - l2_transport - Модуль, реализующий приём/передачу пакетов
## Примерное представление программы
![Optional Text](misc/diagram.svg)