//
// Created by leonardo on 20/05/20.
//

#include <string.h>
#include <sys/socket.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>

#define MSG_SIZE 512

/*
 * Invia il messaggio contenuto nel buffer sulla socket desiderata.
 */
void send_msg(int socket, const char *msg) {
    int ret;
    char msg_to_send[MSG_SIZE];
    sprintf(msg_to_send, "%s\n", msg);
    int bytes_left = strlen(msg_to_send);
    int bytes_sent = 0;
    while (bytes_left > 0) {
        ret = send(socket, msg_to_send + bytes_sent, bytes_left, 0);
        if (ret == -1 && errno == EINTR) continue;
        if(ret < 0){
            perror("Error during send_msg operation!");
            exit(EXIT_FAILURE);
        }

        bytes_left -= ret;
        bytes_sent += ret;
    }
}

/*
 * Riceve un messaggio dalla socket desiderata e lo memorizza nel
 * buffer buf di dimensione massima buf_len bytes.
 *
 * La fine di un messaggio in entrata è contrassegnata dal carattere
 * speciale '\n'. Il valore restituito dal metodo è il numero di byte
 * letti ('\n' escluso), o -1 nel caso in cui il client ha chiuso la
 * connessione in modo inaspettato.
 */

size_t recv_msg(int socket, char *buf, size_t buf_len) {
    int ret;
    int bytes_read = 0;

    // messaggi più lunghi di buf_len bytes vengono troncati
    while (bytes_read <= buf_len) {
        ret = recv(socket, buf + bytes_read, 1, 0);

        if (ret == 0) return -1; // il client ha chiuso la socket
        if (ret == -1 && errno == EINTR) continue;
        if(ret < 0){
            perror("Error during recv_msg operation!");
            exit(EXIT_FAILURE);
        }

        // controllo ultimo byte letto
        if (buf[bytes_read] == '\n') break; // fine del messaggio: non incrementare bytes_read

        bytes_read++;
    }

    /* Quando un messaggio viene ricevuto correttamente, il carattere
     * finale '\n' viene sostituito con un terminatore di stringa. */
    buf[bytes_read] = '\0';
    return bytes_read; // si noti che ora bytes_read == strlen(buf)
}
