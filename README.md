# STLibraryChat

A Particle project named STLibraryChat

## Welcome to your project!

## Как использовать
    - Склонируйте репозиторий на свой локальный компьютер.
    - Убедитесь, что у вас установлен компилятор C++. Данный код компилировался и создавался с помощью [Clang 16.0.0 arm64-apple-darwin22.1.0]
    - Для корректной сборки проекта установите CMAKE и добавьте в свой файл launch.json следующие абзацы кода:

        {
            "type": "lldb",
            "request": "launch",
            "name": "Launch_client_1",
            "program": "${workspaceFolder}/build/bin/myclient",
            "args": [],
            "cwd": "${workspaceFolder}"
        },
        {
            "type": "lldb",
            "request": "launch",
            "name": "Launch_client_2",
            "program": "${workspaceFolder}/build/bin/myclient",
            "args": [],
            "cwd": "${workspaceFolder}"
        }, 
    
    они создадут вам 2 лаунчера, что-бы вы могли запустить двух клиентов одновременно.
    Все необходимые файлы проекта для Cmake сборки уже есть.

    - Соберите проект с помощью компилятора C++. Для этого необходимо выполнить команды Configure и Build.

    - Откройте терминал вашего IDE -> войдите в директорию <cd build> -> <./bin/myserver 8080> либо другой порт который не занят вашей OC. Таким образом вы запустите сервер который слушает порт 8080. Далее в вашем IDE запустите лаунчеры клиентов которые по умолчанию настроены на порт 8080.

    - Введите команды согласно описанию для взаимодействия с сервером.

    - Примечание: Для корректной работы сервера убедитесь, что серверная часть также запущена и доступна по указанному IP-адресу и порту.

#### ```main.cpp``` file:

Это серверный код проекта. Он обеспечивает функциональность для обработки подключений клиентов, аутентификации пользователей, отправки сообщений и широковещательной рассылки.

# Использование
    Сервер умеет обрабатывать 4 команды:

    COMMAND_LOGIN - авторизация
    COMMAND_REGISTER - регистрация
    COMMAND_SEND - отправка сообщения конкретному пользователю
    COMMAND_BROADCAST - отправка сообщения всем пользователям одновременно

    Порядок действий: не важен, но вам обязательно необходимо пройти авторизацию с помощью команды COMMAND_LOGIN -> COMMAND_REGISTER или наоборот. Это важно потому что команда COMMAND_LOGIN запускает поток, который опрашивает сервер на предмет новых сообщений.

##### ######################### Класс User #####################################
    Класс User представляет подключенного пользователя и имеет следующие атрибуты:

    socket: целое число, представляющее сокет пользователя
    name: строка, представляющая имя пользователя
    password: строка, представляющая пароль пользователя
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
# Глобальные переменные
    В коде сервера определены следующие глобальные переменные:

    users: вектор указателей на объекты класса User, представляющих подключенных пользователей
    usersMutex: мьютекс, используемый для обеспечения потокобезопасного доступа к вектору users
_ _ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _
# Вспомогательные функции
     Cервера включает несколько вспомогательных функций:

     int find_user_socket(const std::string& name)
        Эта функция ищет пользователя с указанным именем в векторе users. Она возвращает сокет пользователя, если пользователь найден, или -1, если пользователь не найден.
    
    void sendMessage(int socket, const std::string& message)
        Эта функция отправляет сообщение на указанный сокет. Она добавляет символ новой строки ('\n') к сообщению перед отправкой.

    std::string receiveMessage(int client_socket)
        Эта функция получает сообщение с указанного клиентского сокета. Она считывает символы из сокета до тех пор, пока не встретит символ новой строки ('\n'), и возвращает полученное сообщение в виде строки.

    std::vector<std::string> split(const std::string& str, const std::string& delimiter)
        Эта функция разделяет строку на подстроки с использованием указанного разделителя и возвращает вектор подстрок.
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
# Функции команд
    Код сервера предоставляет следующие функции команд:

    bool cmd_login(int socket, const std::string& name, const std::string& password)
        Эта функция выполняет аутентификацию пользователя при входе в систему. Она проверяет, существует ли пользователь с указанным именем и паролем в векторе users. Если аутентификация успешна, она отправляет клиенту сообщение "STATUS_OK"; в противном случае отправляется сообщение "STATUS_UNAUTHORIZED".
    
    void cmd_register(int socket, const std::string& name, const std::string& password)
        Эта функция регистрирует нового пользователя с указанным именем и паролем. Она добавляет нового пользователя в вектор users и отправляет клиенту сообщение "STATUS_OK".

    void cmd_send(int socket, const std::string& sender, const std::string& recipient, const std::string& text)
        Эта функция осуществляет поиск сокета получателя и отправляет ему сообщение.

    void cmd_broadcast(int socket, const std::string& sender, const std::string& text)
        Эта функция отправляет широковещательное сообщение от пользователя с указанным сокетом всем подключенным пользователям, кроме отправителя.
_ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ __ _ _ _ _ _ _ _ _ _ __ 
# Основной цикл сервера
    Основной цикл сервера ожидает подключений клиентов и создает новый поток для каждого подключения. Каждый поток обрабатывает команды, поступающие от соответствующего клиента.

#### ```client.cpp``` file:

Класс Client представляет клиентское соединение с сервером. Он отвечает за установку соединения, отправку и получение сообщений от сервера, а также выполнение основного цикла программы.

##### ######################### Класс Client #####################################

# Методы класса
    Client(const std::string &server_ip, int server_port): Конструктор класса, принимает IP-адрес сервера и порт сервера в качестве параметров. Создает объект клиента с указанными параметрами.

    ~Client(): Деструктор класса, закрывает соединение с сервером при уничтожении объекта Client.
    void connectToServer(): Устанавливает соединение с сервером.

    void sendMessage(const std::string &message, bool silent = false): Отправляет сообщение на сервер. Принимает сообщение в виде строки и флаг, указывающий, нужно ли выводить информацию об отправляемом сообщении.

    std::string receiveMessage(): Читает сообщение от сервера побайтно до символа новой строки и возвращает полученное сообщение.

    void closeConnection(): Закрывает соединение с сервером.

    void receiveMessages(): Потоковая функция для периодического опроса сервера на наличие новых сообщений. Получает сообщения, выводит их на консоль и выполняет дополнительную обработку.

    void run(): Основной цикл выполнения клиентской программы.

# Основной код программы
    В функции main создается объект Client с указанным IP-адресом и портом сервера. Затем устанавливается соединение с сервером и запускается основной цикл программы, который ожидает ввода команды от пользователя и выполняет соответствующие действия.

    Подробное описание каждой команды и их обработка приведены в коде.