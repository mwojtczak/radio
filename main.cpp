#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <boost/algorithm/string.hpp>
#include <unistd.h>
#include <iostream>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include "err.h"
#include <netdb.h>
#include <sys/poll.h>
#include <map>
#include <boost/algorithm/string.hpp>
#include <map>
#include <boost/algorithm/string.hpp>#include <map>
#include <boost/algorithm/string.hpp>#include <map>
#include <boost/algorithm/string.hpp>#include <map>
#include <boost/algorithm/string.hpp>#include <map>
#include <boost/algorithm/string.hpp>

#define YES "yes"
#define NO "no"
#define PAUSE "PAUSE"
#define PLAY "PLAY"
#define TITLE "TITLE"
#define QUIT "QUIT"
#define MAX_COMMAND_LENGHT 5
#define DO_NOT_SAVE_IN_FILE "-"
#define COMMAND_LENGHT   5
#define PORT_NUM     10001
#define WAITING_TIME_IN_SECONDS 5
#define BUFFER_SIZE 100000
#define MAX_HEADER_SIZE 100000
#define AVERAGE_HEADER_LENGHT 500 //@TODO: sprawdzić : przynamniej żeby było  < to + 8000

#define EMPTY_STATE 0
#define PLAY_STATE 1
#define TITLE_STATE 2
#define PAUSE_STATE 3
#define QUIT_STATE 4

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
int res;
int state;
std::map<std::string, std::string> m;
int metadata_byte_len = 0;
int counter;

int convert_param_to_number(string num, string param_name) {
    for (size_t i = 0; i < num.length(); ++i) {
        if (!isdigit(num[i])) {
            fatal("Użycie:  ./player host path r-port file m-port md, parametr %s nie jest liczbą.\n",
                  param_name.c_str());
        }
    }
    return stoi(num);
}

bool yes_no_check(string word) {
    if (word.compare(YES) == 0) {
        return true;
    } else if (word.compare(NO) == 0) {
        return false;
    } else {
        fatal("Użycie: ./player host path r-port file m-port md, parametr md = %s nie jest słowem 'yes' lub 'no'.\n",
              word.c_str());
    }
    return false;
}

ofstream *open_file() {
    if (file_param.compare(DO_NOT_SAVE_IN_FILE) == 0)
        return NULL;
    ofstream file;
    file.open(file_param);
    if (!file.is_open()) {
        fatal("Error opening file\n");
    }
    return &file;
}

void save_to_file(ofstream file, char *buffer, size_t size) {
    if (file == NULL) {
        //@TODO: cout
    } else {
        file.write(buffer, size);
        if (file.bad()) {
            fatal("Error wtiting to file\n");
        }
    }
}

void close_file(ofstream file) {
    if (file == NULL)
        return;
    file.close();
    if (file.bad()) {
        fatal("Error wtiting to file\n");
    }
}


void check_parameters(int argc, char **argv) {
    if (argc != 7) {
        fatal("Użycie:  ./player host path r-port file m-port md\n");
    }
    host_param = argv[1];
    path_param = argv[2];
    radio_port_param = convert_param_to_number(argv[3], "r-port");
    file_param = argv[4];
    udp_port_param = convert_param_to_number(argv[5], "m-port");
    metadata = yes_no_check(argv[6]);
}

void connect_to_server() {
    string request = "GET ";
    request.append(path_param).append(" HTTP /1.0\r\n");
    if (metadata)
        request.append("Icy-MetaData:1\r\n");
    request.append("\r\n");
    cout << request << endl;
    if (write(sock, request.c_str(), request.length()) != request.length()) {
        syserr("partial / failed write");
    }
}

void parse_header(string s) {
    std::istringstream resp(s);
    std::string header;
    std::string::size_type index;
    while (std::getline(resp, header) && header != "\r") {
        index = header.find(':', 0);
        if (index != std::string::npos) {
            m.insert(std::make_pair(
                    boost::algorithm::trim_copy(header.substr(0, index)),
                    boost::algorithm::trim_copy(header.substr(index + 1))
            ));
        }
    }

    for (auto &kv: m) {
        std::cout << "KEY: `" << kv.first << "`, VALUE: `" << kv.second << '`' << std::endl;
    }
}

void play_command() {
    cout << "play\n";
//    if (state == EMPTY_STATE) {
        connect_to_server();
//    }
    state = PLAY_STATE;
}

void pause_commnd() {
    cout << "pause\n";
    state = PAUSE_STATE;
}

void title_command() {
    cout << "title\n";
}

void quit_command() {
    //@TODO: udp się samo zamknie, a co z tcp?
    exit(EXIT_SUCCESS);
}

void do_command(string command) {
    command = command.substr(0, command.length() - 1); //@TODO: usunąć przy oddawaniu zadania: do testów w konsoli
    if (command.compare(PLAY) == 0)
        play_command();
    else if (command.compare(PAUSE) == 0)
        pause_commnd();
    else if (command.compare(TITLE) == 0)
        title_command();
    else if (command.compare(QUIT) == 0)
        quit_command();
    else {
        fprintf(stderr, "Nieznane polecenie %s.\n", command.c_str());
    }
}

void get_metadata(char * buffer, int size){
    //@TODO
    printf("METADATA: %zd bytes: %s\n", size, buffer);
}

void perform_mp3_data(char * buffer, int size){
    //@TODO
    if (state == PLAY_STATE) {
        printf("read from socket: %zd bytes: %s\n", size, buffer);
    }
}

void read_metadata(){
    unsigned char byte;
    int metadata_size;
    rcv_len = read(sock, &byte, 1);
    metadata_size = byte;
    if (rcv_len == -1)
        fatal("read");
    if (rcv_len == 0){
        //serwer nie odpowiada, poczekać trochę czy zakonczyć?
    }
    metadata_size = metadata_size * 16;
    cout <<endl<< metadata_size <<endl;
    rcv_len = read(sock, buffer, metadata_size); //@TODO: chyba trzeba zrobić odczytywanie whilem, gdyby przesłali mniej
    get_metadata(buffer, rcv_len);
}

//counter: przeczytaliśmy counter po metadanych
void prepare_data(char * buffer){
    int position = 0; //ilość przeczytanych pozycji z bufora
    int metadata_len = 0;
    if (counter < metadata_byte_len){
        rcv_len = read(sock, buffer, metadata_byte_len - counter); //+-1
        counter += rcv_len;
        printf("dane: %zd bytes: %s\n", rcv_len, buffer);
    }
    if (counter == metadata_byte_len){
        //sprintf len to metadata
        read_metadata();
        counter = 0;
    }
}

int main(int argc, char **argv) {

    check_parameters(argc, argv);

    int udp_sock;
    int flags, sflags;
    struct sockaddr_in udp_server_address;
    struct sockaddr_in udp_client_address;

    char command[COMMAND_LENGHT + 1];
    socklen_t snda_len, rcva_len;
    ssize_t udp_read_len, udp_send_len;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (udp_sock < 0)
        syserr("socket");
    // after socket() call; we should close(udp_sock) on any execution path;
    // since all execution paths exit immediately, udp_sock would be closed when program terminates

    udp_server_address.sin_family = AF_INET; // IPv4
    udp_server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    udp_server_address.sin_port = htons(
            PORT_NUM); // default port for receiving is PORT_NUM @TODO: zmienić na udp_port_param

    // bind the socket to a concrete address
    if (bind(udp_sock, (struct sockaddr *) &udp_server_address, (socklen_t) sizeof(udp_server_address)) < 0) {
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

    snda_len = (socklen_t) sizeof(udp_client_address);
//    play_command();
    struct pollfd fd[2];

    state = EMPTY_STATE;
    //@TODO: przeplot PAUSE PLAY: nie ma połączenia bo łaczy gdy na początku jest stan EMPTY
    fd[0].fd = udp_sock;
    fd[0].events = POLLIN | POLLOUT;
    fd[1].fd = sock;
    fd[1].events = POLLIN | POLLOUT;

    bool header_ended = false;
    string http_header = "";

    int end_of_header = 0;
    //int counter = 0; // mod metadata_byte_len
    int metadata_len = 0;

    counter = 0;
    while ((res = poll(fd, 2, 1000)) >= 0) {
        if (res > 0) {
            if (fd[0].revents & POLLIN) { //udp
                rcva_len = (socklen_t) sizeof(udp_client_address);
                flags = 0; // we do not request anything special
                udp_read_len = recvfrom(udp_sock, command, sizeof(command), flags,
                                        (struct sockaddr *) &udp_client_address, &rcva_len);
                if (udp_read_len < 0) {
                    syserr("error on datagram from client socket");
                } else {
                    //@TODO: do command
                    //w buforze command jest otrzymane polecenie
                    std::string command_string(command, udp_read_len); //przy oddawaniu zadania
                    cout << command_string.length() << endl;


                    do_command(command_string);

                    (void) printf("read from socket: %zd bytes: %.*s\n", udp_read_len,
                                  (int) udp_read_len, command);

                }
            }

            if (fd[1].revents & POLLIN) { //tcp
//                cout << " Z SERVERA ICY\n";
                if (header_ended) {
                        prepare_data(buffer);
//                    if (counter < metadata_byte_len) {
//                        //doczytaj date do metabajtu
                        //rcv_len = read(sock, buffer, sizeof(buffer)); //+-1
//                        counter += rcv_len;
//                        //zrob coś z danymi
//                        if (state == PLAY_STATE) {
//                            printf("JESTread from socket: %zd bytes: %s\n", rcv_len, buffer);
//                        }
//
//                    }
//                    if (counter == metadata_byte_len) {
//                        rcv_len = read(sock, &metadata_len, 1);
//                        metadata_len = 16 * metadata_len; // @TODO: sprawdzić jaki jest przelicznik
//                        //przeczytaj metadane
//                        rcv_len = read(sock, buffer, metadata_len); //+-1
//                        counter = 0;
//
//                    }
                } else {
                    if ((rcv_len = read(sock, buffer, AVERAGE_HEADER_LENGHT)) > 0) {
                        string read_data(buffer, rcv_len);
                            if ((end_of_header = read_data.find("\r\n\r\n")) >= read_data.length()) { //nie znaleziono
//                cout << read_data;
                                if (http_header.length() < MAX_HEADER_SIZE) //żeby nie zapełnić pamięci??
                                    http_header.append(read_data);
                            } else {
                                cout << "\nFOUND STH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                                string header = read_data.substr(0, end_of_header + 3);
                                cout << header;
                                http_header.append(header);
                                cout << "\nEND OF HEADER HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                                cout << "FULL HEADER: \n";
                                cout << http_header << "END OF HEADER\n";
                                parse_header(http_header);
                                metadata_byte_len = stoi(m["icy-metaint"]);
                                header_ended = true;
                                //@TODO w buforze na buffer + end_of_header + 4 (perwsza pozycja) są dane do przetworzenia
                                if (counter + read_data.substr(end_of_header + 4).length() > metadata_byte_len) {
                                    cout << endl<< metadata_byte_len << "\nMETADANE W BUFOrZE"<< counter<< "  "<< read_data.substr(end_of_header + 4).length() << "  "  << metadata_byte_len << "\n";
                                } else {
                                    int test = *((int*)buffer + end_of_header + 4);
                                    cout << "\nJESZCZE NIE MA METADANYCH OK\n" << test << "\n";
                                    counter = read_data.substr(end_of_header + 4).length();
                                    counter = rcv_len - end_of_header - 4;
                                    unsigned char byte = *(buffer + end_of_header + 4);
                                    int teest = byte;
                                    if (*(buffer + end_of_header) == '\r')
                                        cout << "TAK\n";
                                    cout << byte << endl << sizeof(int) << "  "<< sizeof(char )<< endl << *(buffer + end_of_header + 4)<< endl << "teest" << teest * 16<< endl;
                                }
                                if (state == PLAY_STATE) {
                                    cout << read_data.substr(end_of_header + 4);
                                }

                            }
                        if (rcv_len < 0) {
                            syserr("read");
                        }
                    } // rcv read
                }

            } //if tcp
        } //IF RES
    }
    if (res == -1) {
        fatal("poll");
    }
    cout << "Hello, World!" << endl;
    return 0;
}