#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
void print_menu() {
printf("\nAvailable commands:\n");
printf(" STORE <doc_id> <client_name> <content> - Create a new document\n");
printf(" UPDATE <doc_id> <client_name> <content> - Update an existing
document\n");
printf(" DELETE <doc_id> <client_name> - Delete a document\n");
printf(" READ <doc_id> <client_name> - Read a document\n");
printf(" exit - Disconnect from server\n");
}
int main() {
int sock;
struct sockaddr_in server_addr;
char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
// Создание сокета
sock = socket(AF_INET, SOCK_STREAM, 0);
if (sock == -1) {
perror("Socket creation failed");
exit(EXIT_FAILURE);
}
// Настройка адреса сервера
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(PORT);
if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
perror("Invalid server address");
close(sock);
exit(EXIT_FAILURE);
}
// Подключение к серверу
if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
perror("Connection failed");
close(sock);
exit(EXIT_FAILURE);
}
printf("Connected to server at %s:%d\n", SERVER_IP, PORT);
while (1) {
print_menu();
printf("Enter command: ");
// Очистка буфера и ввод команды
memset(buffer, 0, BUFFER_SIZE);
fgets(buffer, BUFFER_SIZE, stdin);
buffer[strcspn(buffer, "\n")] = 0; // Удаляем символ новой строки
// Команда для завершения работы
if (strcmp(buffer, "exit") == 0) {
printf("Disconnecting from server.\n");
break;
}
// Проверка пустого ввода
if (strlen(buffer) == 0) {
printf("Error: Command cannot be empty. Try again.\n");
continue;
}
// Отправка команды на сервер
if (send(sock, buffer, strlen(buffer), 0) == -1) {
perror("Failed to send data");
break;
}
// Получение ответа от сервера
memset(response, 0, BUFFER_SIZE);
ssize_t bytes_received = recv(sock, response, BUFFER_SIZE, 0);
if (bytes_received <= 0) {
printf("Server disconnected.\n");
break;
}
// Отображение ответа от сервера
printf("Server response: %s\n", response);
}
// Закрытие сокета
close(sock);
return 0;
}
