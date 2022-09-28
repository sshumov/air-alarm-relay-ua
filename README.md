[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua/)

# air-alarm-relay-ua

Реле включения уличного освещения базируясь на расписании и оповещении о воздушной тревоге.

В качестве исполнительного устройства был взять контроллер [KMA-111](https://www.bakler.com.ua/shop/wifi_energy_counter/kma_111_40_cn.html)

Причина - оно у меня было, и вообще нравятся устройства. И да - сейчас они работают !
На борту esp8266, реле на 40ампер, часі реального времени(не использую), счетчик электроэнергии(не использую)


### Основная идея 
Контроллер работает по примитивному расписани состоящему из 24 байтов [0|1], соответсвенно от 00:00 часов до 23:59. 0 - реле выключено, 1 - реле включено

Одновременно происходит мониторинг состояния воздушной тревоги по регионам Украины.
Протокол оповещения взят Alerts Ukraine (https://alerts.com.ua/). Можно использовать как этот сервис, так и самостоятельно запустить сервер оповещения на основе этого протокола. [raid](https://github.com/and3rson/raid)


### Запуск и работа
При длительном нажатии кнопки на лицевой стороне устройсва (10 сек) реле переходит в режим точки доступа и ожидает подключения по адресу 192.168.4.1.

Так-же это меню доступно при работе реле по адресу /portal

![меню контроллера](https://github.com/sshumov/air-alarm-relay-ua/raw/master/img/airrelay.png)

Для защиты меню следует указать пароль авторизации для пользователя **admin** в поле Wifi password и установить чекбокс Protect

Для проверки статуса воздушной тревоги следует установить чекбокс Check AIR alert и указать необходимы параметры

#### Старт

При старте/ребуте состояние реле - ВЫКЛ.

В случае если в настройках включена проверка на воздушную тревогу:
- по умолчанию устанавливаем состояние тревоги true (это предотвратит включение реле)
- запускаем таск проверки состояния с переодичностью укказанной в настройках (советую делать ее в пределах минуты)

#### Работа
- раз в минуту производится проверка флажка тревоги, в случае активности - немедленно выключаем реле
- если по какой-то причине нет доступа к АПИ тревоги - отключаем реле 
- если нет тревоги и в этот час активировано реле - включаем его


