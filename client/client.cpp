#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

class Client //определяем класс которой представляет клиентское соединение с сервером.
{
public:
    Client(const std::string &server_ip, int server_port)
        : server_ip_(server_ip), server_port_(server_port), client_socket_(-1), is_connected_(false) {}

    ~Client() {
        closeConnection(); // метод закрывает соединение с сервером при уничтожении объекта Client.
    }

    void connectToServer()
    {
        try
        {
            createSocket(); // создаем сокет

            server_address_.sin_family = AF_INET; // устанавливаем тип и
            server_address_.sin_port = htons(server_port_); // устанавливаем порт серверного адреса.

            if (inet_pton(AF_INET, server_ip_.c_str(), &server_address_.sin_addr) <= 0) // IP-адрес сервера преобразуется из текстового формата в структуру sockaddr_in с помощью функции inet_pton().
            {
                throw std::runtime_error("Error: invalid server IP address");
            }

            if (connect(client_socket_, (struct sockaddr *)&server_address_, sizeof(server_address_)) < 0) // подключаемся к серверу
            {
                throw std::runtime_error("Error: failed to connect to server");
            }

            is_connected_ = true; // еслиподключение прошло успешно, устанавливается флаг is_connected_ в значение true.

        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    // функция которая используется для отправки сообщения на сервер.
    void sendMessage(const std::string &message, bool silent = false) // передаем ссылку на константную строку и флаг, указывающий, нужно ли выводить информацию об отправляемом сообщении.
    {
        try
        {
            if (!is_connected_) { // проверяем подключен ли клиент к серверу
                throw std::runtime_error("Error: client is not connected to server");
            }
            
            std::string message_str = message + "\n"; // создаем сроку message_str, которая содержит сообщение + символ новой строки (\n), чтобы разделить сообщения на сервере.
            if(!silent) { // если флаг silent равен false, то сообщение выводится на консоль с помощью std::cout.
                std::cout << "Sending message: " << message_str;
            }

            int nbytes = 0; // инициализируем счетчик отправленных байт
            while (nbytes < message_str.length()) { // в каждой итерации цикла отправляется часть сообщения
                int result = send(client_socket_, message_str.c_str() + nbytes, message_str.length() - nbytes, 0);
                if (result < 0) {
                    throw std::runtime_error("Failed to send message");
                }
                nbytes += result; // колличество успешно отправленных байтов nbytes обновляется после каждой итерации цикла.
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    // функция которая читает сообщение от сервера побайтно до символа новой строки и сохраняет его в строке message. Когда встречается символ новой строки, это означает, что команда завершена, и функция возвращает полученное сообщение.
    std::string receiveMessage() { // не принимает никаких параметров и возвращает строку.
        std::string message; // создаем строку

        for (;;) { // созлаем цикл который будет продолжаться до тех пор, пока не будет прочитан символ новой строки ('\n').
            char c = 0;
            // в каждой итерации цикла происходит:
            int nbytes = recv(client_socket_, &c, 1, 0); // читается один символ из сокета с помощью функции recv(). Результат чтения сохраняется в переменную nbytes.
            if (nbytes < 0)
                return "";

            if (nbytes == 1 && c != '\n')           
                message.append(&c, 1); // если был прочитан один байт (nbytes == 1) и этот байт не является символом новой строки (c != '\n'), он добавляется к строке message.

            if (c == '\n') // если был прочитан символ новой строки (c == '\n'), это означает конец команды, и цикл прерывается.
                break;
        }
        return message; // возвращаем полученное сообщение
    }

    void closeConnection()
    {
        if (client_socket_ != -1)
        {   
            shutdown(client_socket_, SHUT_RDWR);
            close(client_socket_);
            client_socket_ = -1;
            is_connected_ = false;
        }
    }

    // Потоковая функция для периодического опроса сервера на наличие новых сообщений, получает их, выводит на консоль и выполняет дополнительную обработку (если предусмотрено).
    void receiveMessages() {
        //std::cout << "Received message from start!!! " << std::endl;
        while (true) {
            try 
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // устанавливаем интервал между запросами на получение новых сообщений.

                // Отправка запроса на получение новых сообщений от сервера
                std::string response;
                {
                    std::lock_guard<std::mutex> l(socket_guard);
                    sendMessage("COMMAND_CHECK_MESSAGES", true); // отправляется запрос на сервер используя функцию sendMessage. Флаг true указывает на то, что сообщение должно быть отправлено без вывода на консоль.
                    // Получение ответа от сервера
                    while (true) {
                        response = receiveMessage(); // ответ сохраняется в переменной response.
                        if (response.substr(0, 9) == "STATUS_NO") {
                            break; // если префикс ответа response равен "STATUS_NO" (длина префикса равна 9), цикл прерывается с помощью break. Это означает, что больше нет сообщений для получения, и переходим к следующей итерации внешнего цикла while (true).
                        }
                        //std::cout << "Received message from server: " << response << std::endl;
                        std::cout << response << std::endl;
                    }
                }
                    //std::cout << "Received message from server: " << response << std::endl;
                // Обработка полученного сообщения и передача его клиенту
                // ...

            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                break;
            }
        }
    }

    // функция обработки которая отвечает за основной цикл выполнения клиентской программы.
    void run()
    {
        while (true) {
            try 
            {
                // Получение сообщения от пользователя
                std::string message;
                std::cout << "Enter command: ";
                std::getline(std::cin, message); // получаем сообщение
                std::cout << "message: " << message << std::endl;

                // Обработка команды "COMMAND_EXIT"
                if (message == "COMMAND_EXIT") {
                    break;
                }

                // Обработка команды "COMMAND_LOGIN"
                if (message == "COMMAND_LOGIN") {

                    // Получение имени пользователя
                    std::string name;
                    std::cout << "Enter name: ";
                    std::getline(std::cin, name);

                    // Получение пароля пользователя
                    std::string password;
                    std::cout << "Enter password: ";
                    std::getline(std::cin, password);

                    // Отправка запроса на авторизацию на сервер
                    std::string request = "COMMAND_LOGIN|" + name + "|" + password;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);
                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                    
                    // Проверяем, успешно ли выполнена команда "COMMAND_LOGIN"
                    {
                        std::thread receive_thread(&Client::receiveMessages, this); // создается отдельный поток receive_thread, который вызывает функцию receiveMessages для получения сообщений от сервера.
                        receive_thread.detach();  // Отсоединяем поток от главного потока клиента
                    }   // эта процедура позволяет выполнять получение сообщений параллельно с обработкой пользовательских команд.
                    
                }
                // Обработка команды "COMMAND_REGISTER"
                else if (message == "COMMAND_REGISTER") {
                    // Получение имени пользователя
                    std::string name;
                    std::cout << "Enter name: ";
                    std::getline(std::cin, name);

                    // Получение пароля пользователя
                    std::string password;
                    std::cout << "Enter password: ";
                    std::getline(std::cin, password);

                    // Формирование запроса на регистрацию пользователя
                    std::string request = "COMMAND_REGISTER|" + name + "|" + password;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
                // Обработка команды "COMMAND_SEND"
                else if (message == "COMMAND_SEND") {

                    // Получение имени получателя
                    std::string recipient;
                    std::cout << "Enter recipient name: ";
                    std::getline(std::cin, recipient);

                    // Получение текста сообщения
                    std::string text;
                    std::cout << "Enter message: ";
                    std::getline(std::cin, text);

                    // Формирование запроса на отправку сообщения
                    std::string request = "COMMAND_SEND|" + recipient + "|" + text;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
                // Обработка команды "COMMAND_BROADCAST"
                else if (message == "COMMAND_BROADCAST") {

                    // Получение текста сообщения для широковещательной рассылки
                    std::string text;
                    std::cout << "Enter message: ";
                    std::getline(std::cin, text);
                
                    // Формирование запроса на широковещательную рассылку сообщения
                    std::string request = "COMMAND_BROADCAST|" + text;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                break;
            }
        }
        closeConnection();
    }   

private:
    // Создание сокета
    void createSocket()
    {
        if ((client_socket_ = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            throw std::runtime_error("Error: failed to create client socket");
        }
    }
    std::string server_ip_;
    int server_port_;
    int client_socket_;
    bool is_connected_;
    struct sockaddr_in server_address_;
    std::mutex socket_guard;
};

int main()
{
    std::string server_ip = "127.0.0.1";
    int server_port = 8080;

    Client client(server_ip, server_port);

    try
    {
        client.connectToServer();
        std::cout << "Connected to server\n";
        client.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
