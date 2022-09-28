[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua/)

# air-alarm-relay-ua

Реле включения уличного освещения базируясь на расписании и оповещении о воздушной тревоге.

### Оборудование
В качестве исполнительного устройства был взять контроллер [KMA-111](https://www.bakler.com.ua/shop/wifi_energy_counter/kma_111_40_cn.html)

Причина - оно у меня было, и вообще нравятся устройства. И да - сейчас они работают !
На борту esp8266, реле на 40ампер, часі реального времени(не использую), счетчик электроэнергии(не использую), бистабильное реле

Технически, с небольшим изменением кода возможно использование различных устройств (sonoff И тд)


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




# air-alarm-relay-ua

Relay to turn on street lights based on schedule and air alarm notification.

### Equipment.
I used [KMA-111](https://www.bakler.com.ua/shop/wifi_energy_counter/kma_111_40_cn.html) as my actuator.

Reason - I had it, and generally like devices. And yes - now they work !
On board esp8266, 40amp relay, real time clock(not used), electricity meter(not used), bistable relay

Technically with a small change of code it is possible to use different devices (sonoff And so on)


### The basic idea 
The controller operates on a primitive schedule consisting of 24 bytes [0|1], respectively, from 00:00 hours to 23:59. 0 - relay off, 1 - relay on

At the same time airborne alarm conditions are monitored by regions of Ukraine.
Alerts protocol is taken from Alerts Ukraine (https://alerts.com.ua/). You can use both this service and run your own alert server based on this protocol. [raid](https://github.com/and3rson/raid)


### Startup and operation
When you press the button on the front panel of the device for a long time (10 sec) the relay switches to the access point mode and waits for a connection to 192.168.4.1.

This menu is also available during relay operation via /portal

![controller menu](https://github.com/sshumov/air-alarm-relay-ua/raw/master/img/airrelay.png)

To protect the menu, specify the authorization password for the user **admin** in the field Wifi password and set the Protect checkbox

To check the status of air alarm set the Check AIR alert checkbox and specify the necessary parameters

#### Start

During start/restart the relay status is OFF.

In case the check for air alarm is enabled in the settings:
- by default, set the alarm state to true (this will prevent the relay from turning on)
- Run the check cycle (I advise to set it to less than one minute)

#### Operation
- check the alarm flag once a minute, and if it is active, immediately turn off the relay
- if, for some reason, there is no access to the alarm API - deactivate the relay 
- if there is no alarm and the relay is active at that hour - switch it on

*** Translated with www.DeepL.com/Translator (free version) ***

