#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string>
#include <map>
#include <string.h>
#include <unistd.h> // close function
#include <fcntl.h> // open function
#include <netinet/tcp.h>
#include <bits/stdc++.h>

#define BUFFLEN 1700


struct udp_packet {
    char topic[50];
    char tip_date;
    char content[1500];
    char ip[16];
    char port[6];
    char client_id[50];

};
struct tcp_packet{
    int type;
    char content[65];
};
int main(int argc, char **argv){
        
    setvbuf(stdout, NULL, _IONBF, BUFFLEN);

    char buf[BUFFLEN];
    std::string client_id;
    std::string topic;

    //map intre client_id si socket-ul acestuia
    std::map <std::string, int> clients_socks;
    // multime in care avem mapati la un client mai multe topic uri --> (client) -> {(topic1), (topic2), (topic3)}
    std::map <std::string, std::unordered_set<std::string>> subscriptions;
    // multime de aceeasi forma in care pastram topic-urile pentru sf
    std::map <std::string, std::unordered_set<std::string>> sf_subs;
    //storage-ul
    struct udp_packet non_sent_packets[20];
    int non_sent_packets_counter = 0;

    uint16_t port = atoi(argv[1]);
    
    struct sockaddr_in sv_addr = {0};
    struct sockaddr_in client_addr = {0};
    socklen_t addrlen = 0;
    struct udp_packet udp_message = {0};

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    sv_addr.sin_port = htons(port);


    // socket udp si tcp
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int new_tcp_sockfd;
    
    if(udp_sockfd < 0){
        perror("UDP Socket\n");
        exit(EXIT_FAILURE);
    }
    if(tcp_sockfd < 0){
        perror("TCP Socket\n");
        exit(EXIT_FAILURE);
    }

    // bind udp_socket la localhost si la portul dat ca parametru
    int rc = bind(udp_sockfd, (const sockaddr*)&sv_addr, sizeof(struct sockaddr_in));    
    
    if(rc < 0){
        perror("UDP bind\n");
        close(udp_sockfd);
        exit(EXIT_FAILURE);
    }

    //bind tcp_socket la localhost si la portul dat ca parametru
    rc = bind(tcp_sockfd, (const sockaddr*)&sv_addr, sizeof(struct sockaddr_in));    
    
    if(rc < 0){
        perror("TCP bind\n");
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }
    // disbale Nagle Algorithm
    int no_delay = 1;
    rc = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&no_delay, sizeof(int));

    // pun tcp_scoket sa asculte conextiuni noi
    rc = listen(tcp_sockfd,1);
    if(rc < 0){
        perror("Listen\n");
        exit(EXIT_FAILURE);
    }

    fd_set read_fds_set;
    fd_set fds;
    int fd_max;
    int new_cli_fd;
    FD_ZERO(&read_fds_set);
    FD_ZERO(&fds);

    // link fds la fd_set
    FD_SET(STDIN_FILENO,&fds);
    FD_SET(udp_sockfd,&fds);
    FD_SET(tcp_sockfd,&fds);
    fd_max = tcp_sockfd;



    while(1){
        // update la read_fds_set cu eventuali noi clienti care vor fi "pusi" in fds
        read_fds_set = fds;
        // asteptam input pe oricare dintre socketi
        if(select(fd_max + 1, &read_fds_set, NULL, NULL, NULL) == -1){
            perror("Select\n");
            exit(EXIT_FAILURE);
        }
         if (FD_ISSET(STDIN_FILENO, &read_fds_set)) {
            std::string stdin_buf;
            getline(std::cin,stdin_buf);
            
            if (stdin_buf.find("exit") != std::string::npos) {
                struct udp_packet closing_pack = {0};
                closing_pack.tip_date = 4;

                // fd trebuie sa inceapa de la 5 ca sa nu inchida stdin, stdout etc;
                for(int fd = 5; fd < fd_max + 1; fd++) {
                    send(fd, (char*)&closing_pack, sizeof(struct udp_packet), 0);
                    close(fd);
                }
                break;
            } 
        }


        for(int fd = 1; fd < fd_max +1; fd++){
            // exista un fd gata pentru citire
            if(FD_ISSET (fd,&read_fds_set)){
                // new connection
                if(fd == tcp_sockfd){
                    addrlen = sizeof(struct sockaddr_in);
                    if((new_cli_fd = accept(tcp_sockfd, (struct sockaddr*) &client_addr, &addrlen)) == -1){
                        perror("Accept\n");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(new_cli_fd, &fds);
                    if(new_cli_fd > fd_max)
                        fd_max = new_cli_fd;
                }
                else if(fd == udp_sockfd){
                    memset(buf, 0, BUFFLEN);
                    int n = recvfrom(udp_sockfd, buf, sizeof(struct udp_packet), 0, (struct sockaddr*)&client_addr, &addrlen);
                    memcpy(&udp_message,buf,sizeof(struct udp_packet));
                    if (n < 0){
                        perror("UDP Recvfrom\n");
                        exit(EXIT_FAILURE);
                    }
                    memcpy(&udp_message.topic,buf,50);
                    memcpy(&udp_message.tip_date,buf+50,sizeof(uint8_t));
                    if(udp_message.tip_date == 0){
                        uint8_t semn;
                        // copy dupa topic si tipul de date
                        memcpy(&semn, buf + 51 , sizeof(uint8_t));
                        uint32_t num;
                        // copy dupa topic, tip de date si semn
                        memcpy(&num, buf + 52, sizeof(uint32_t));
                        num = ntohl(num);
                        
                        std::string s_num = "";
                        if(semn){
                            s_num = "-";
                            s_num += std::to_string(num);
                        } else{
                            s_num = std::to_string(num);
                        }
                        // pun in buf ul de continut al mesajului rezultatul
                        memcpy(&udp_message.content, s_num.c_str(), s_num.length());

                    } else if(udp_message.tip_date == 1){
                        uint16_t short_num;
                        // copy dupa short num
                        memcpy(&short_num, buf + 51, sizeof(uint16_t));
                        short_num = ntohs(short_num);
                        // numarul e inmultit cu 100, motiv pt care il impartim la 100
                        float fl_num = (float)short_num / 100;
                        // pun in buf ul de continut rezultatul cu 2 zecimale
                        sprintf(udp_message.content, "%.2f", fl_num);

                    } else if(udp_message.tip_date == 2){
                        uint8_t semn;
                        // copy dupa semn
                        memcpy(&semn, buf + 51 , sizeof(uint8_t));

                        uint32_t num;
                        // copy dupa modulul nr
                        memcpy(&num, buf + 52, sizeof(uint32_t));
                        num = ntohl(num);
                        uint8_t pow_10;
                        // copy dupa putere
                        memcpy(&pow_10, buf + 56, sizeof(uint8_t));
                        double div_num = (double)num;
                        // impartim la 10 in functie de puterea nr
                        for(uint8_t i = 0; i < pow_10; i++){
                            div_num /= 10;
                        }
                        if(semn){
                            div_num *= -1;
                        }
                        sprintf(udp_message.content,"%.*f",pow_10, div_num);

                    } else if(udp_message.tip_date == 3){
                        memcpy(udp_message.content,buf + 51, BUFFLEN);
                    }

                    // send content to subscribers
                    for (const auto  &pair: subscriptions)
                    {
                        // daca e vreun subscriber la vreunul din topicurile primite pe socketul udp
                        // trimit la toti subscriberii abonati
                        if(clients_socks.count(pair.first) && pair.second.count(udp_message.topic)){
                            int send_sock = clients_socks.at(pair.first);
                            int n = send(send_sock, (char*)&udp_message, sizeof(struct udp_packet), 0);
                            
                        } else if(pair.second.count(udp_message.topic)){
                            memcpy(&non_sent_packets[non_sent_packets_counter],(char*)&udp_message,sizeof(struct udp_packet));
                            non_sent_packets_counter++;   
                        }
                    }
                    
                    
                }
                else {
                    struct tcp_packet tcp_msg;
                    int n = recv(fd, &tcp_msg, sizeof(struct tcp_packet), 0);
                    if(n < 0){
                        continue;
                    }
                    // clientul a intrerupt conexiunea si il scoatem din set
                    if(n == 0){
                        close(fd);
                        FD_CLR(fd, &fds);
                    } else {
                        // cineva incearca sa se conecteze
                        if(tcp_msg.type == 0){

                            // daca clientul este deja conectat 
                            if(clients_socks.find(tcp_msg.content) != clients_socks.end()){
                                struct udp_packet arleady_connected_msg = {0};
                                arleady_connected_msg.tip_date = 4;
                                // trimit inapoi connection closed
                                sendto(fd,(char*)&arleady_connected_msg, sizeof(struct udp_packet), 0, (const sockaddr*)&sv_addr, sizeof(sv_addr));
                                printf("Client %s already connected.\n", tcp_msg.content);
                                break;
                            }
                            
                            // send content to subscribed clients that were 
                            // disconnected when new contend came up
                            for (const auto  &pair: sf_subs)
                            {
                                for(int k = 0; k < non_sent_packets_counter; k++){
                                    // trimitem la clientul reconectact mesajele stocate
                                    if(pair.second.count(non_sent_packets[k].topic)){
                                        int n = send(fd, (char*)&non_sent_packets[k], sizeof(struct udp_packet), 0);
                                    }
                                }
                                
                            }        

                            // pun in multimea de clienti un nou fd
                            printf("New client %s connected from .\n", tcp_msg.content);
                            clients_socks[tcp_msg.content] = fd;
                            

                        }
                        if(tcp_msg.type == 1){
                            // clientul a inchis conexiunea si il scoatem din set
                            FD_CLR(fd, &fds);
                            fd_max -= 1;
                            clients_socks.erase(tcp_msg.content);
                            printf("Client %s disconnected.\n",tcp_msg.content);
                            continue;
                        } else if(tcp_msg.type == 2){
                            // daca clientul vrea sa se aboneze
                            // il punem in subscriptions alaturi de topicul la care vrea sa se aboneze
                            std::string data(tcp_msg.content);
                            std::stringstream content(data);
                            std::string topic;
                            std::string type_of_subscription;
                            std::string client_id;
                            content >> client_id;
                            content >> topic;
                            content >> type_of_subscription;
                            
                            // stocam topic-urile pentru sf
                            if(std::stoi(type_of_subscription) == 1){
                                    std::unordered_set <std::string> storage;
                                    sf_subs.insert(std::pair<std::string, std::unordered_set<std::string>>(client_id,storage));     
                                    sf_subs.at(client_id).insert(topic);                           
                            }

                            if(subscriptions.count(client_id) == 0){
                                std::unordered_set <std::string> v;
                                subscriptions.insert(std::pair<std::string, std::unordered_set<std::string>>(client_id,v));
                                
                            };
                            subscriptions.at(client_id).insert(topic);




                        } else if(tcp_msg.type == 3){
                            //daca clientul vrea sa se aboneze ii stergem topicul la care este abonat din subscriptions
                            std::string data(tcp_msg.content);
                            std::stringstream content(data);
                            std::string client_id;
                            std::string topic;
                            content >> client_id;
                            content >> topic;
                            subscriptions.at(client_id).erase(topic);
                        }
                    }
                        
    
                }
            }
        }
    }
    close(udp_sockfd);
    close(tcp_sockfd);
    
    return 0;
}