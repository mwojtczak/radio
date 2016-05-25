#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "err.h"

#define COMMAND_LENGHT   1000
#define PORT_NUM     10001

using namespace std;

string host_param;  //nazwa serwera udostępniającego strumień audio;
string path_param;  //nazwa zasobu, zwykle sam ukośnik;
int radio_port_param;     //numer portu serwera udostępniającego strumień audio,
string file_param;  //file   – nazwa pliku, do którego zapisuje się dane audio,
// a znak minus, jeśli strumień audio ma być wysyłany na standardowe
//wyjście (w celu odtwarzania na bieżąco);
int udp_port_param; //m-port – numer portu UDP, na którym program nasłuchuje poleceń,
bool metadata;      //no, jeśli program ma nie żądać przysyłania metadanych, yes wpp

int convert_param_to_number(string num, string param_name){
    for (size_t i = 0; i < num.length(); ++i) {
        if (!isdigit(num[i])){
            fatal("Użycie:  ./player host path r-port file m-port md, parametr %s nie jest liczbą.\n", param_name.c_str());
        }
    }
    return stoi(num);
}

bool yes_no_check(string word){
    if (word.compare("yes") == 0){
        return true;
    }else if(word.compare("no") == 0){
        return false;
    } else {
        fatal("Użycie: ./player host path r-port file m-port md, parametr md = %s nie jest słowem 'yes' lub 'no'.\n", word.c_str());
    }
    return false;
}

void check_parameters(int argc, char** argv){
    if (argc != 7){
        fatal("Użycie:  ./player host path r-port file m-port md\n");
    }
    host_param = argv[1];
    path_param = argv[2];
    radio_port_param = convert_param_to_number(argv[3], "r-port");
    file_param = argv[4];
    udp_port_param = convert_param_to_number(argv[5], "m-port");
    metadata = yes_no_check(argv[6]);
}

void do_command(string command){
    cout << command;
}

int main(int argc, char** argv) {

    check_parameters(argc, argv);

    int udp_sock;
    int flags, sflags;
    struct sockaddr_in udp_server_address;
    struct sockaddr_in udp_client_address;

    char command[COMMAND_LENGHT];
    socklen_t snda_len, rcva_len;
    ssize_t udp_read_len, udp_send_len;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (udp_sock < 0)
        syserr("socket");
    // after socket() call; we should close(udp_sock) on any execution path;
    // since all execution paths exit immediately, udp_sock would be closed when program terminates

    udp_server_address.sin_family = AF_INET; // IPv4
    udp_server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    udp_server_address.sin_port = htons(PORT_NUM); // default port for receiving is PORT_NUM @TODO: zmienić na udp_port_param

    // bind the socket to a concrete address
    if (bind(udp_sock, (struct sockaddr *) &udp_server_address, (socklen_t) sizeof(udp_server_address)) < 0){
        syserr("bind");
    }

    snda_len = (socklen_t)sizeof(udp_client_address);
    for (;;) {
        do {
            rcva_len = (socklen_t) sizeof(udp_client_address);
            flags = 0; // we do not request anything special
            udp_read_len = recvfrom(udp_sock, command, sizeof(command), flags,
                                    (struct sockaddr *) &udp_client_address, &rcva_len);
            if (udp_read_len < 0) {
                syserr("error on datagram from client socket");
            }
            else {
                //@TODO: do command
                //w buforze command jest otrzymane polecenie
                std::string command_string (command, udp_read_len - 1);
                cout << command_string << endl;
                do_command(string(command));
                (void) printf("read from socket: %zd bytes: %.*s\n", udp_read_len,
                              (int) udp_read_len, command);
//                sflags = 0;
//                udp_send_len = sendto(udp_sock, command, (size_t) udp_read_len, sflags,
//                                 (struct sockaddr *) &udp_client_address, snda_len);
//                if (udp_send_len != udp_read_len)
//                    syserr("error on sending datagram to client socket");
            }
        } while (udp_read_len > 0);
        (void) printf("finished exchange\n");
    }

    cout << "Hello, World!" << endl;
    return 0;
}