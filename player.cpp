#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include "err.h"
#include <netdb.h>
#include <sys/poll.h>
#include <ctime>
#include <boost/algorithm/string.hpp>

#define YES "yes"
#define NO "no"
#define PAUSE "PAUSE"
#define PLAY "PLAY"
#define TITLE "TITLE"
#define QUIT "QUIT"
#define DO_NOT_SAVE_IN_FILE "-"
#define COMMAND_LENGHT   5
#define WAITING_TIME_IN_SECONDS 5
#define BUFFER_SIZE 100000
#define AVERAGE_HEADER_LENGHT 300
#define MAX_HEADER_SIZE 26000 + AVERAGE_HEADER_LENGHT
#define ICY_NAME "ICY"

#define EMPTY_STATE 0
#define PLAY_STATE 1
#define PAUSE_STATE 2
#define EXIT_FAILURE 1

using namespace std;

string host_param;               //nazwa serwera udostępniającego strumień audio;
string path_param;               //nazwa zasobu, zwykle sam ukośnik;
int radio_port_param;            //numer portu serwera udostępniającego strumień audio,
string file_param;               //file – nazwa pliku, do którego zapisuje się dane audio,
int udp_port_param;              //m-port – numer portu UDP, na którym program nasłuchuje poleceń,
bool metadata;                   //'no', jeśli program ma nie żądać przysyłania metadanych, 'yes' wpp
int sock;
struct addrinfo addr_hints;
struct addrinfo *addr_result;
int err;
char buffer[BUFFER_SIZE];
ssize_t rcv_len;
int res;
int state;
std::map<std::string, std::string> m;
int metadata_byte_len = 0;
int counter;
string latest_title = "";
int udp_sock;
int flags;
struct sockaddr_in udp_server_address;
struct sockaddr_in udp_client_address;
bool save = false;
ofstream file;
std::clock_t last_server_call;

int convert_param_to_number(string num, string param_name) {
    try {
        return stoi(num);
    }
    catch (const std::invalid_argument &ia) {
        fatal("Użycie:  ./player host path r-port file m-port md, parametr %s nie jest liczbą.\n",
              param_name.c_str());
    }
    return stoi(num);
}

/*
 * Funkcja sprawdza, czy odpowiedz serwera jest zgodna z protokołem ICY
 *
 * @param: string reprezentujący pierwsze trzy znaki odpowiedzi serwera
 */
void check_http_response_type(string name){
    if (name.compare(ICY_NAME) != 0){
        fatal("Wrong http response");
    }
}

/*
 * Funkcja sprawdza, czy HTTP request kończy się właściwym kodem.
 * Dopuszczalne kody, które obsługujemy to 200, 302, 304.
 *
 * @param: napis reprezentujący ów kod
 */
void check_returned_http_code(string code_str) {
    int code;
    try {
        code = stoi(code_str);
    }
    catch (const std::invalid_argument &ia) {
        fatal("bad http code");
    }
    if (!(code == 200 || code == 302 || code == 304)) {
        fatal("bad http code");
    }
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

void open_file() {
    if (file_param.compare(DO_NOT_SAVE_IN_FILE) == 0)
        return;
    file.open(file_param);
    if (!file.is_open()) {
        fatal("Error opening file\n");
    }
}

void save_to_file(ofstream *file, const char *buffer, size_t size) {
    if (file == NULL) {
        fatal("Error handling file\n");
    } else {
        (*file).write(buffer, size);
        if ((*file).bad()) {
            fatal("Error wtiting to file\n");
        }
    }
}

void close_file() {
    if (file)
        return;
    file.close();
    if (file.bad()) {
        fatal("Error wtiting to file\n");
    }
}

/*
 * Funkcja sprawdza ilość i poprawność parametrów.
 */
void check_parameters(int argc, char **argv) {
    if (argc != 7) {
        fatal("Użycie:  ./player host path r-port file m-port md\n");
    }
    host_param = argv[1];
    path_param = argv[2];
    radio_port_param = convert_param_to_number(argv[3], "r-port");
    file_param = argv[4];
    if (file_param.compare(DO_NOT_SAVE_IN_FILE) != 0)
        save = true;
    udp_port_param = convert_param_to_number(argv[5], "m-port");
    metadata = yes_no_check(argv[6]);
}

/*
 * Funkcja buduje zapytanie HTTM, a następnie przesyła je do serwera podanego w parametrach wywołania programu.
 */
void connect_to_server() {
    string request = "GET ";
    request.append(path_param).append(" HTTP /1.0\r\n");
    if (metadata)
        request.append("Icy-MetaData:1\r\n");
    request.append("\r\n");
    if ((size_t)write(sock, request.c_str(), request.length()) != request.length()) {
        syserr("partial / failed write");
    }
}

void get_all_values(){
    for (auto &kv: m) {
        std::cout << "KEY: `" << kv.first << "`, VALUE: `" << kv.second << '`' << std::endl;
    }
}

/*
 * Funkcja parsuje header http, wyciągając z niego wartości parametrów i zapisując w mapie.
 *
 * @param: napis do sparsowania, reprezentujący header http
 */
void parse_header(string s) {
    check_returned_http_code(s.substr(4, 3));
    check_http_response_type(s.substr(0, 3));
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
}

string parse_metadata(string meta) {
    string metadata(meta);
    metadata.replace(0, strlen("StreamTitle="), "");
    metadata = metadata.substr(0, metadata.find_first_of(";"));
    int len = metadata.length();
    if (len >= 2)
        return metadata.substr(1, metadata.length() - 2);
    return metadata;
}

void play_command() {
    state = PLAY_STATE;
}

void pause_commnd() {
    state = PAUSE_STATE;
}

void title_command() {
    int sflags = 0;
    size_t snd_len = 0;
    if (latest_title.length() > 0) {
        snd_len = sendto(udp_sock, latest_title.c_str(), (size_t) latest_title.length(), sflags,
                         (struct sockaddr *) &udp_client_address, (socklen_t) sizeof(udp_client_address));
    }
    if (snd_len != latest_title.length())
        syserr("error on sending datagram to client socket");
}

void quit_command() {
    close(sock);
    close_file();
    exit(EXIT_SUCCESS);
}

void do_command(string command) {
    if (command[command.length() - 1] == '\n')    
        command = command.substr(0, command.length() - 1);
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

string get_metadata(char *buffer, int size) {
    string metadata(buffer, size);
    string title = parse_metadata(metadata);
    return title;
}

void perform_mp3_data(const char *buffer, int size) {
    if (state == PLAY_STATE) {
        if (save) {
            save_to_file(&file, buffer, size);
        } else {
            fwrite(buffer, 1, size, stdout);
        }
    }
}

/*
 * Funkcja czyta metadane z bufora i je przetwarza.
 *
 * Odczytuje z socketu bajt długości segmentu metadanych, a następnie odczytuje metadane o tej długości powiększonej 16 razy.
 * Następnie przetwarza odczytane metadane, wyciągając z nich informację o tytule obecnego utworu.
 */
void read_metadata() {
    unsigned char byte;
    int metadata_size;
    rcv_len = read(sock, &byte, 1);
    metadata_size = byte;
    if (rcv_len == -1)
        fatal("read");
    if (rcv_len == 0) {
        quit_command();
    }
    metadata_size = metadata_size * 16;
    if (metadata_size > 0) {
        int read_len = 0;
        while (read_len < metadata_size) {
            rcv_len = read(sock, buffer + read_len, metadata_size - read_len);
            if (rcv_len == 0) {
                quit_command();
            }
            read_len += rcv_len;
        }
        latest_title = get_metadata(buffer, rcv_len);
    }
}

/*
 * Funkcja czyta dane z socketu i rozdziela dane od metadanych na podstawie miejsca wystąpienia
 * a następnie przetwarza je.
 *
 * @param: wskaźnik na miejsce w pamięci, gdzie można zapisywać dane
 *
 * @info: counter jest ilością dotychczas przeczytanych bajtów
 */
void prepare_data(char *buffer) {
    if (counter < metadata_byte_len) {
        rcv_len = read(sock, buffer, metadata_byte_len - counter);
        counter += rcv_len;
        perform_mp3_data(buffer, rcv_len);
    }
    if (counter == metadata_byte_len) {
        read_metadata();
        counter = 0;
    }
}

int find_icy_metaint() {
    if (m.find("icy-metaint") == m.end()) {
        fatal("No icy-metaint in header");
    }
    return stoi(m["icy-metaint"]);
}

//-1 brak headera, wpp. ilość danych przeczytanych po headerze
//zakładamy że nie przeczytamy metadanych tutaj
/*
 * Funkcja bada, czy zbudowany do tej pory header jest kompletny, badając czy zawiera zakończenie "\r\n\r\n"
 *
 * @param: zbudowany dotychczas header
 *
 * @return: -1 gdy header jest niekompletny,
 *          wpp. ilość przeczytanych bajtów danych po headerze
 */
int find_header_end(string http_header) {
    size_t end_of_header = 0;
    if ((end_of_header = http_header.find("\r\n\r\n")) >= http_header.length()) {
        if (http_header.length() > MAX_HEADER_SIZE) {
            fatal("Too long header.\n");
        }
        return -1;
    } else {
        string header = http_header.substr(0, end_of_header + 3);
        parse_header(header);
        if (metadata){
            metadata_byte_len = find_icy_metaint();
        }
        string some_data(http_header.substr(end_of_header + 4), http_header.length() - end_of_header - 4);
        counter = http_header.length() - end_of_header - 4;
        perform_mp3_data(some_data.c_str(), some_data.length());
        return http_header.length() - end_of_header - 4;
    }
}

void prepare_data_without_metadata(char *buffer) {
    rcv_len = read(sock, buffer, sizeof(buffer) - 1);
    perform_mp3_data(buffer, rcv_len);
}

void exit_failure(){
    close(sock);
    close_file();
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    check_parameters(argc, argv);
    char command[COMMAND_LENGHT + 1];

/************************************ UDP connection *****************************************************************/
    socklen_t rcva_len;
    ssize_t udp_read_len;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0)
        syserr("socket");

    udp_server_address.sin_family = AF_INET; // IPv4
    udp_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_server_address.sin_port = htons(udp_port_param);

    if (bind(udp_sock, (struct sockaddr *) &udp_server_address, (socklen_t) sizeof(udp_server_address)) < 0) {
        syserr("bind");
    }
/*************************************** TCP connection **************************************************************/

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    addr_hints.ai_flags = AI_PASSIVE;
    err = getaddrinfo(host_param.c_str(), to_string(radio_port_param).c_str(), &addr_hints, &addr_result);
    if (err == EAI_SYSTEM) {
        syserr("getaddrinfo: %s", gai_strerror(err));
    }
    else if (err != 0) {
        fatal("getaddrinfo: %s", gai_strerror(err));
    }

    sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0) {
        syserr("socket");
    }
    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
        syserr("connect");
    }

    freeaddrinfo(addr_result);

/******************************************* poll() setup ************************************************************/

    struct pollfd fd[2];

    state = PLAY_STATE;
    fd[0].fd = udp_sock;
    fd[0].events = POLLIN | POLLOUT;
    fd[1].fd = sock;
    fd[1].events = POLLIN | POLLOUT;

    bool header_ended = false;
    string http_header = "";
    open_file();
    counter = 0;
/****************************************** connecting to server *****************************************************/
    connect_to_server();
    last_server_call = std::clock();

    while ((res = poll(fd, 2, 1000 * WAITING_TIME_IN_SECONDS)) >= 0) {
        if (res == 0){
            fprintf(stderr, "Przekroczono czas oczekiwania na dane od serwera radiowego.\n");
            exit_failure();
        }
        if (res > 0) {
            if (( std::clock() - last_server_call ) / (double) CLOCKS_PER_SEC > WAITING_TIME_IN_SECONDS){
                fprintf(stderr, "Przekroczono czas oczekiwania na dane od serwera radiowego.\n");
                exit_failure();
            }

            if (fd[0].revents & POLLIN) {
                rcva_len = (socklen_t) sizeof(udp_client_address);
                flags = 0;
                udp_read_len = recvfrom(udp_sock, command, sizeof(command), flags,
                                        (struct sockaddr *) &udp_client_address, &rcva_len);
                if (udp_read_len < 0) {
                    syserr("error on datagram from client socket");
                } else {
                    std::string command_string(command, udp_read_len);
                    do_command(command_string);
                }
            }
            if (fd[1].revents & POLLIN) {
                last_server_call = clock();
                if (header_ended) {
                    if (metadata) {
                        prepare_data(buffer);
                    } else {
                        prepare_data_without_metadata(buffer);
                    }
                } else {
                    if ((rcv_len = read(sock, buffer, AVERAGE_HEADER_LENGHT)) > 0) {
                        string read_data(buffer, rcv_len);
                        http_header.append(read_data);
                        int read_len = 0;
                        if ((read_len = find_header_end(http_header)) > -1) {
                            header_ended = true;
                            counter = read_len;
                        }
                        if (rcv_len < 0) {
                            syserr("read");
                        }
                    }
                    if (rcv_len== 0){
                        quit_command();
                    }
                }
            }
        }
    }
    if (res == -1) {
        fatal("poll");
    }
    close_file();
    return 0;
}
