#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

#include "serializacion.h" 

t_header obtener_header(int fd){
    int header;
    int bytes_recv = recv(fd, &header, sizeof(int), 0);
    
    if (bytes_recv == -1) return -1;
    if (bytes_recv == 0)  return -1;
    if (bytes_recv < sizeof(int)) return -1;
    
    return header;
}

void* obtener_payload(int fd){
    int size_payload;
    int bytes_recv = recv(fd, &size_payload, sizeof(int), 0);

    if (bytes_recv == -1) return NULL;
    if (bytes_recv == 0)  return NULL;
    if (bytes_recv < sizeof(int)) return NULL;
    
    
    void* payload = malloc(size_payload);
    bytes_recv = recv(fd, payload, size_payload, 0);

    if (bytes_recv == -1) return NULL;
    if (bytes_recv == 0)  return NULL;
    if (bytes_recv < size_payload) return NULL;
    
    return payload;
}