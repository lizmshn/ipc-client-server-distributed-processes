#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#define PORT 8080
#define MAX_DOCUMENTS 1024
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
// Структура документа
typedef struct {
char id[50];
char content[BUFFER_SIZE];
char owner[50];
char permissions[MAX_CLIENTS][50];
int permissions_count;
} Document;
Document documents[MAX_DOCUMENTS];
int document_count = 0;
sem_t semaphore; // Глобальный семафор
// Поиск документа по ID
Document* find_document(const char* id) {
for (int i = 0; i < document_count; i++) {
if (strcmp(documents[i].id, id) == 0) {
return &documents[i];
}
}
return NULL;
}
// Проверка прав доступа
bool check_permissions(Document* doc, const char* client) {
if (strcmp(doc->owner, client) == 0) return true;
for (int i = 0; i < doc->permissions_count; i++) {
if (strcmp(doc->permissions[i], client) == 0) return true;
}
return false;
}
// Обработка клиентских запросов
void* handle_client(void* client_sock_ptr) {
int client_sock = *(int*)client_sock_ptr;
free(client_sock_ptr);
char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
while (1) {
memset(buffer, 0, BUFFER_SIZE);
memset(response, 0, BUFFER_SIZE);
// Получение данных от клиента
ssize_t bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
if (bytes_received <= 0) {
break;
}
// Парсинг команды
char command[10], doc_id[50], client[50], content[BUFFER_SIZE];
sscanf(buffer, "%s %s %s %[^\n]", command, doc_id, client, content);
sem_wait(&semaphore); // Блокировка доступа к документам
if (strcmp(command, "STORE") == 0) { // Хранение документа
if (find_document(doc_id)) {
snprintf(response, BUFFER_SIZE, "ERROR: Document %s already exists\n",
doc_id);
} else {
strcpy(documents[document_count].id, doc_id);
strcpy(documents[document_count].content, content);
strcpy(documents[document_count].owner, client);
documents[document_count].permissions_count = 0;
document_count++;
snprintf(response, BUFFER_SIZE, "OK: Document %s stored\n", doc_id);
}
} else if (strcmp(command, "UPDATE") == 0) { // Изменение документа
Document* doc = find_document(doc_id);
if (!doc) {
snprintf(response, BUFFER_SIZE, "ERROR: Document %s not found\n",
doc_id);
} else if (!check_permissions(doc, client)) {
snprintf(response, BUFFER_SIZE, "ERROR: Permission denied\n");
} else {
strcpy(doc->content, content);
snprintf(response, BUFFER_SIZE, "OK: Document %s updated\n", doc_id);
}
} else if (strcmp(command, "DELETE") == 0) { // Удаление документа
Document* doc = find_document(doc_id);
if (!doc) {
snprintf(response, BUFFER_SIZE, "ERROR: Document %s not found\n",
doc_id);
} else if (!check_permissions(doc, client)) {
snprintf(response, BUFFER_SIZE, "ERROR: Permission denied\n");
} else {
*doc = documents[--document_count];
snprintf(response, BUFFER_SIZE, "OK: Document %s deleted\n", doc_id);
}
} else if (strcmp(command, "READ") == 0) { // Чтение документа
Document* doc = find_document(doc_id);
if (!doc) {
snprintf(response, BUFFER_SIZE, "ERROR: Document %s not found\n",
doc_id);
} else if (!check_permissions(doc, client)) {
snprintf(response, BUFFER_SIZE, "ERROR: Permission denied\n");
} else {
snprintf(response, BUFFER_SIZE, "OK: Document %s content: %s\n", doc_id,
doc->content);
}
} else {
snprintf(response, BUFFER_SIZE, "ERROR: Unknown command\n");
}
sem_post(&semaphore); // Разблокировка доступа к документам
// Отправка ответа клиенту
send(client_sock, response, strlen(response), 0);
}
close(client_sock);
return NULL;
}
int main() {
int server_sock, *client_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t client_addr_len = sizeof(client_addr);
// Инициализация семафора
sem_init(&semaphore, 0, 1);
// Создание серверного сокета
server_sock = socket(AF_INET, SOCK_STREAM, 0);
if (server_sock == -1) {
perror("Socket creation failed");
exit(EXIT_FAILURE);
}
// Настройка адреса сервера
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(PORT);
// Привязка сокета
if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
perror("Bind failed");
close(server_sock);
exit(EXIT_FAILURE);
}
// Прослушивание соединений
if (listen(server_sock, 5) == -1) {
perror("Listen failed");
close(server_sock);
exit(EXIT_FAILURE);
}
printf("Server is running on port %d\n", PORT);
while (1) {
// Принятие клиента
client_sock = malloc(sizeof(int));
*client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
if (*client_sock == -1) {
perror("Accept failed");
free(client_sock);
continue;
}
// Обработка клиента в отдельном потоке
pthread_t thread;
pthread_create(&thread, NULL, handle_client, client_sock);
pthread_detach(thread);
}
// Уничтожение семафора
sem_destroy(&semaphore);
close(server_sock);
return 0;
}
