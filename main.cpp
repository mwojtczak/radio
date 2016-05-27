#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define YES "yes"
#define NO "no"
#define PAUSE "PAUSE"
#define PLAY "PLAY"
#define TITLE "TITLE"
#define QUIT "QUIT"
#define DO_NOT_SAVE_IN_FILE "-"
#define COMMAND_LENGHT   1000
#define PORT_NUM     10001
#define WAITING_TIME_IN_SECONDS 5
#define BUFFER_SIZE 100000

using namespace std;

string host_param;  //nazwa serwera udostępniającego strumień audio;
string path_param;  //nazwa zasobu, zwykle sam ukośnik;
int radio_port_param;     //numer portu serwera udostępniającego strumień audio,
string file_param;  //file   – nazwa pliku, do którego zapisuje się dane audio,
// a znak minus, jeśli strumień audio ma być wysyłany na standardowe
//wyjście (w celu odtwarzania na bieżąco);
int udp_port_param; //m-port – numer portu UDP, na którym program nasłuchuje poleceń,
bool metadata;      //no, jeśli program ma nie żądać przysyłania metadanych, yes wpp
int sock;
struct addrinfo addr_hints;
struct addrinfo *addr_result;

int i, err;
char buffer[BUFFER_SIZE];
ssize_t len, rcv_len;

int convert_param_to_number(string num, string param_name){
    for (size_t i = 0; i < num.length(); ++i) {
        if (!isdigit(num[i])){
            fatal("Użycie:  ./player host path r-port file m-port md, parametr %s nie jest liczbą.\n", param_name.c_str());
        }
    }
    return stoi(num);
}

bool yes_no_check(string word){
    if (word.compare(YES) == 0){
        return true;
    }else if(word.compare(NO) == 0){
        return false;
    } else {
        fatal("Użycie: ./player host path r-port file m-port md, parametr md = %s nie jest słowem 'yes' lub 'no'.\n", word.c_str());
    }
    return false;
}

ofstream * open_file(){
    if (file_param.compare(DO_NOT_SAVE_IN_FILE) == 0)
        return NULL;
    ofstream file;
    file.open (file_param);
    if (!file.is_open()){
        fatal("Error opening file\n");
    }
    return &file;
}

void save_to_file(ofstream file, char * buffer, size_t size){
    if (file == NULL){
        //@TODO: cout
    } else {
        file.write(buffer, size);
        if (file.bad()){
            fatal("Error wtiting to file\n");
        }
    }
}

void close_file(ofstream file){
    if (file == NULL)
        return;
    file.close();
    if (file.bad()){
        fatal("Error wtiting to file\n");
    }
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


//wyszukuje jane zakonczone na /r/n
int get_data(){

}

void play_command(){
    cout << "play\n";
    string request = "GET ";
    request.append(path_param).append(" HTTP /1.0\r\n");
    if (metadata)
        request.append("Icy-MetaData:1\r\n");
    request.append("\r\n");
    if (write(sock,  request.c_str(), request.length()) != request.length()) {
        syserr("partial / failed write");
    }

    memset(buffer, 0, sizeof(buffer));
    int head_len = 197;
//    rcv_len = read(sock, buffer, sizeof(buffer) - 1);
//    if (rcv_len < 0) {
//        syserr("read");
//    }
//    printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
////    if (write(sock,  request.c_str(), request.length()) != request.length()) {
////        syserr("partial / failed write");
////    }
//    rcv_len = read(sock, buffer, 300);
//    if (rcv_len < 0) {
//        syserr("read");
//    }
    bool header_ended = false;
    string http_header = "";
    while ((rcv_len = read(sock, buffer, sizeof(buffer) - 1)) > 0){
        if (rcv_len < 0) {
            syserr("read");
        }
        string read_data (buffer, rcv_len);
        int end_of_header = 0;
        if (!header_ended){
//            while ((end_of_header = read_data.find("\r\n\r\n")) < read_data.length()){ //nie znaleziono
//
//                cout << "\nFOUND STH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
//                string header = read_data.substr(0, end_of_header + 1);
//                cout << header;
//                cout << "\nEND OF HEADER HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
//                cout << read_data.substr(end_of_header + 1);
//                read_data = read_data.substr(end_of_header + 1);
////                header_ended = true;
////                break;
//            }
//            cout << read_data;
            if ((end_of_header = read_data.find("\r\n\r\n")) > read_data.length()){ //nie znaleziono
                cout << read_data;
                http_header.append(read_data);
            }else {
                cout << "\nFOUND STH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                string header = read_data.substr(0, end_of_header + 1);
                cout << header;
                http_header.append(header);
                cout << "\nEND OF HEADER HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                cout << "FULL HEADER: \n";
                cout << http_header << "END OF HEADER\n";
                cout << read_data.substr(end_of_header + 1);
//                header_ended = true;
//                break;
            }

        } else {

            printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
        }
    }
    //printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
//    rcv_len = read(sock, buffer, sizeof(buffer) - 1);
//    if (rcv_len < 0) {
//        syserr("read");
//    }
//    printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
}

void pause_commnd(){
    cout << "pause\n";
    while ((rcv_len = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        if (rcv_len < 0) {
            syserr("read");
        }
    }
}

void title_command(){
    //send via udp
//                sflags = 0;
//                udp_send_len = sendto(udp_sock, command, (size_t) udp_read_len, sflags,
//                                 (struct sockaddr *) &udp_client_address, snda_len);
//                if (udp_send_len != udp_read_len)
//                    syserr("error on sending datagram to client socket");
    cout << "title\n";
}

void quit_command(){
    //udp się samo zamknie, a co z tcp?
    exit(EXIT_SUCCESS);
}

void do_command(string command){
    command = command.substr(0, command.length() - 1); //@TODO: usunąć przy oddawaniu zadania: do testów w konsoli
    if (command.compare(PLAY) == 0)
        play_command();
    else if (command.compare(PAUSE) == 0)
        pause_commnd();
    else if (command.compare(TITLE) == 0)
        title_command();
    else if (command.compare(QUIT) == 0)
        quit_command();
    else{
        fprintf (stderr, "Nieznane polecenie. Ignoruję olecenie.\n");
    }
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

/************************* TCP part *********************************************/
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    err = getaddrinfo(host_param.c_str(), argv[3], &addr_hints, &addr_result);
    if (err == EAI_SYSTEM) { // system error
        syserr("getaddrinfo: %s", gai_strerror(err));
    }
    else if (err != 0) { // other error (host not found, etc.)
        fatal("getaddrinfo: %s", gai_strerror(err));
    }

    // initialize socket according to getaddrinfo results
    sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0)
        syserr("socket");

    // connect socket to the server
    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("connect");

    freeaddrinfo(addr_result);

/************************* end of TCP part *********************************************/

    snda_len = (socklen_t)sizeof(udp_client_address);
//    play_command();
    for (;;) {
        do {
            rcva_len = (socklen_t) sizeof(udp_client_address);
            flags = 0; // we do not request anything special
            udp_read_len = recvfrom(udp_sock, command, sizeof(command), flags,
                                    (struct sockaddr *) &udp_client_address, &rcva_len);
            if (udp_read_len < 0) {
                syserr("error on datagram from client socket");
            } else {

                //@TODO: do command
                //w buforze command jest otrzymane polecenie
                std::string command_string (command, udp_read_len); //przy oddawaniu zadania
                cout << command_string.length() << endl;
                do_command(command_string);
                (void) printf("read from socket: %zd bytes: %.*s\n", udp_read_len,
                              (int) udp_read_len, command);

            }
        } while (udp_read_len > 0);
        (void) printf("finished exchange\n");
    }

    cout << "Hello, World!" << endl;
    return 0;
}