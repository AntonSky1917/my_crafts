#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>

class User {
public:
    int socket;
    std::string name;
    std::string password;

    User(int socket, const std::string& name, const std::string& password)
        : socket(socket), name(std::move(name)), password(std::move(password))
    {
    }
};

// определяем глобальные переменные
std::vector<std::shared_ptr<User>> users;
std::mutex usersMutex;

// функция выполняет поиск пользователя по имени в векторе users.
// она принимает аргумент name - имя пользователя, и возвращает соответствующий сокет пользователя (socket) или -1, если пользователь не найден.
int find_user_socket(const std::string& name) {
    std::lock_guard<std::mutex> l(usersMutex); // используется мьютекс usersMutex для обеспечения безопасности доступа к вектору users.
    for (const auto& u : users) {
        if (u->name == name)
            return u->socket;
    }
    return -1;
}

// функция отправляет сообщение на указанный сокет.
void sendMessage(int socket, const std::string& message) {
    std::string message_str = message + "\n"; // добавляет символ новой строки (\n) к сообщению.
    send(socket, message_str.c_str(), message_str.length(), 0);
}

std::string receiveMessage(int client_socket) {
    constexpr std::size_t bufferSize = 4096;
    std::array<char, bufferSize> buffer;
    std::string message;

    for (;;) {
        ssize_t nbytes = recv(client_socket, buffer.data(), 1, 0);
        if (nbytes < 0)
            return "";

        char c = buffer[0];

        if (nbytes == 1 && c != '\n')
            message.append(&c, 1);

        if (c == '\n')
            break;
    }
    return message;
}

// функция разделяет строку str на подстроки, используя разделитель delimiter
void split(const std::string& str, char delimiter, std::vector<std::string>& tokens) {
    size_t start = 0; // инициализируем переменную start
    size_t end = str.find(delimiter); // end инициализируется с помощью функции find(delimiter), которая находит позицию первого вхождения разделителя delimiter в строке str.
    while (end != std::string::npos) { // выполняется цикл, который продолжается до тех пор, пока end не станет равным std::string::npos  - это означает, что разделитель больше не найден.
        // c помощью функции substr() извлекается подстрока от позиции start до позиции end - start, и эта подстрока добавляется в вектор tokens с помощью функции emplace_back().
        tokens.emplace_back(str.substr(start, end - start));
        start = end + 1; // start устанавливается на позицию, следующую сразу за найденным разделителем (end + 1).
        end = str.find(delimiter, start); // переменная end обновляется с помощью функции find(delimiter, start), чтобы найти следующее вхождение разделителя после позиции start.
    }
    tokens.emplace_back(str.substr(start)); // после выхода из цикла добавляется последняя подстрока, начиная с позиции start, с помощью функции emplace_back().
}

// функция выполняет проверку имени пользователя и пароля.
bool cmd_login(int socket, const std::string& name, const std::string& password) {
    bool found = false; // переменная будет использоваться для отслеживания нахождения соответствующего имени пользователя и пароля.

    {
        std::lock_guard<std::mutex> l(usersMutex); // блокировка мьютекса usersMutex. Это необходимо для безопасного доступа к общему ресурсу users (вектор пользователей) из нескольких потоков.
        for (const auto& user : users) { // проходим по всем элементам вектора
            if (user->name == name && user->password == password) {
                found = true; // если соответствие найдено то переменная устанавливается в true.
                break; // и цикл прерывается.
            }
        }
    }

    if (!found) { // если не найдено
        std::cout << "To Client: STATUS_UNAUTHORIZED" << std::endl;
        sendMessage(socket, "STATUS_UNAUTHORIZED"); // отправка сообщения клиенту
    } else {
        std::cout << "To Client: STATUS_OK" << std::endl;
        sendMessage(socket, "STATUS_OK");
    }
    return found; // возвращаем значение переменной
}

// функция выполняет регистрацию нового пользователя.
void cmd_register(int socket, const std::string& name, const std::string& password)
{
    if (find_user_socket(name) >= 0) { // проверяем существует ли уже пользователь с таким именем в списке пользователей.
        std::cout << "To Client: STATUS_ALREADY_REGISTERED" << std::endl;
        sendMessage(socket, "STATUS_ALREADY_REGISTERED"); 
        return;
    }

    {
        std::lock_guard<std::mutex> l(usersMutex);
        // если пользователь с заданным именем не найден, создается новый объект User с использованием переданных параметров socket, name и password.
        users.emplace_back(std::make_shared<User>(std::move(socket), std::move(name), std::move(password)));
    }

    std::cout << "To Client: STATUS_REGISTERED_SUCCESSFULL" << std::endl;
    std::cout << "Registered user: " + name + ", socket: " + std::to_string(socket) << std::endl;
    sendMessage(socket, "STATUS_REGISTERED_SUCCESSFULL");
}

void cmd_send(int socket, const std::string& sender, const std::string& recipient, const std::string& text) {
    std::cout << "Sending message to recipient " << recipient << ": " << text << std::endl;
    int user_socket = find_user_socket(recipient); // поиск сокета получателя

    if (user_socket < 0) { // если не найден
        std::cout << "To Client: STATUS_INVALID_USER" << std::endl;
        // Отправка сообщения о неверном пользователе клиенту
        sendMessage(socket, "STATUS_INVALID_USER");
    } else { // если найден
        std::cout << "To Client: MESSAGE_SENT" << std::endl;
        // Отправка сообщения получателю
        sendMessage(user_socket, std::string("MESSAGE_RECEIVED|") + sender + "|" + text);
        sendMessage(socket, "STATUS_OK"); // сообщение об успешной отправке клиенту
    }
}

// функция выполняет широковещательную рассылку сообщения text от отправителя sender всем пользователям, кроме отправителя.
void cmd_broadcast(int socket, const std::string& sender, const std::string& text) {
    std::vector<int> tosend; // создаем пустой вектор tosend, который будет содержать сокеты всех пользователей, которым нужно отправить сообщение.
    {
        std::lock_guard<std::mutex> l(usersMutex); // блокировка гарантирует, что доступ к вектору users будет синхронизирован между потоками.
        for (const auto& user : users) { // итерируемся по всем пользователям в векторе users.
            tosend.push_back(user->socket); // и сокет каждого пользователя добавляется в вектор tosend.
        }
    }

    auto message = std::string("MESSAGE|") + sender + "|" + text; // создаем строку message

    for (int s : tosend) { // итерируемся по всем сокетам в векторе tosend.
        if (s != socket) // если сокет не совпадает с отправителем.
            sendMessage(s, std::move(message)); // вызываем sendMessage и отправляем сообщение каждому сокету.
    }
    sendMessage(socket, "BROADCAST_SENT");
}

// функция обрабатывает сообщения от клиента, подключенного к серверу.
void handle_user(int client_socket)
{   
    int socket = client_socket; // инициализируем локальную переменную со значением client_socket, переданным в функцию.
    std::string name = "?"; // локальная переменная name со значением "?", которая представляет имя пользователя.
    // зто значение будет обновляться после успешного выполнения команды COMMAND_LOGIN.

    while (true) // цикл будет выполняться до тех пор, пока не будет получено пустое сообщение от клиента (длина равна 0) или пока не произойдет ошибка.
    {   
        // Обработка полученного сообщения от клиента
        std::string command = receiveMessage(socket); // получаем сообщение от клиента.
        std::cout << "Received message from client " << name << ":" << command << std::endl; // выводим сообщение
        if (command.length() == 0) // если длинна = 0 то завершаем обработку пользователя.
            break;

        std::vector<std::string> tokens;
        split(command, '|', tokens); // Используем измененную функцию split с символом '|' в качестве разделителя

        if (tokens.size() == 0) { // Изменили условие проверки на пустоту вектора
            std::cout << "Invalid command" << std::endl;
            continue;
        }

        std::string commandType = tokens[0]; // извлекаем тип команды из первого токена
        if (commandType == "COMMAND_LOGIN") // проверяем первую подстроку на соответствие команде
        {   
            if (cmd_login(socket, tokens[1], tokens[2])) // вызываем функцию cmd_login с передачей ей сокета, логина и пароля.
                name = tokens[1]; // если вход выполнен успешно, значение name обновляется.
        }
        else if(commandType == "COMMAND_REGISTER")
        {
            cmd_register(socket, tokens[1], tokens[2]);
        }
        else if (commandType == "COMMAND_SEND")
        {
            cmd_send(socket, name, tokens[1], tokens[2]);
        }
        else if (commandType == "COMMAND_BROADCAST") 
        {
            // отправить сообщение всем пользователям, за исключением отправителя.
            cmd_broadcast(socket, name, tokens[1]);
        }
        else if (commandType == "COMMAND_CHECK_MESSAGES") {
            sendMessage(socket, "STATUS_NO");
        }
        else
        {
            std::cout << "Invalid command \"" << commandType << "\"" <<std::endl;
            sendMessage(socket, "ERROR_INVALID_COMMAND");
        }
    }

    // Удаление клиента из списка пользователей
    //{
        //std::lock_guard<std::mutex> l(usersMutex);
        //users.erase(std::remove_if(users.begin(), users.end(), [socket](const User* user) {
        //return user->socket == socket;
        //}), users.end());
    //{

    // Но! Точно ли мы должны удалять пользователя? я использую std::remove_if() она перемещает элементы, удовлетворяющие условию, в конец вектора, но не изменяет его размер.
    {
        std::lock_guard<std::mutex> l(usersMutex);
        std::remove_if(users.begin(), users.end(), [socket](const std::shared_ptr<User>& user) {
            return user->socket == socket;
        });
    }

    close(socket);
}

// функция представляет основной цикл сервера, который принимает подключения от клиентов и запускает потоки для обработки каждого подключения.
int main(int argc, char *argv[])
{   
    int port = 8080;

    if (argc >= 2) {
        port = atoi(argv[1]); // читаем порт из командной строки
    }

    int server_socket = socket(PF_INET, SOCK_STREAM, 0); // создаем серверный сокет
    if (server_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // создается структура для хранения информации об адресе сервера.
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) { // сокет сервера связывается с адресом.
        perror("bind");
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }

    if (listen(server_socket, 0) < 0) { // устанавливается прослушивание порта на сервере.
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << port << std::endl;

    std::vector<int> clientSockets; // создаем вектор clientSockets, который будет хранить файловые дескрипторы подключенных клиентов.

    while (true) // запускаем цикл который будет принимать подключения от клиентов.
    {
        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);
        int client_socket = accept(server_socket, (struct sockaddr*)&clientAddress, &addressLength); // вызывается функция accept, которая принимает входящее соединение и возвращает файловый дескриптор клиентского сокета.
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        clientSockets.push_back(client_socket); // файловый дескриптор клиентского сокета добавляется в вектор clientSockets.

        std::cout << "New connection established, socket fd is " << client_socket << std::endl;
        std::thread t(handle_user, client_socket); // создается отдельный поток (std::thread) для обработки пользователя с использованием функции handle_user.
        t.detach(); // далее поток отсоединяется (detach), чтобы продолжить работу независимо от него.
        std::cout << "Client accepted" << std::endl;
    }

    return 0; // когда выполнение достигает конца цикла, программа завершается возвращением значения 0.
}
