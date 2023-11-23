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
#include <iostream>
#include <netinet/tcp.h> 
#include <bits/stdc++.h>

#define BUFFLEN 1700


struct udp_packet {
    char topic[50];
    char tip_date;
    char content[1500];
    uint16_t ip_server;
    char port[6];
    char client_id[50];
};
struct tcp_packet{
    int type;
    char content[65];
};
int main(int argc, char **argv){

    setvbuf(stdout, NULL, _IONBF, BUFFLEN);

    char client_id = *argv[1];
    uint16_t ip_server = atoi(argv[2]);
    uint16_t port = atoi(argv[3]);
    
    struct sockaddr_in sv_addr = {0};

    struct udp_packet udp_message = {0};
    memcpy(udp_message.client_id,argv[1],sizeof(argv[1]));
    memcpy(udp_message.port,argv[3],6);
    udp_message.ip_server = ip_server;


    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(port);
    int rc = inet_aton(argv[2], &sv_addr.sin_addr);
    if(rc < 0){
        perror("Server ip\n");
        exit(EXIT_FAILURE);
    }

    int tcp_sockfd = socket(AF_INET, SOCK_STREAM,0);
    // disable Nagle Algorithm
    int no_delay = 1;
    setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&no_delay, sizeof(int));
    if(tcp_sockfd < 0){

        perror("TCP Socket\n");
        exit(EXIT_FAILURE);
    }
    rc = connect(tcp_sockfd, (const sockaddr*)&sv_addr, sizeof(struct sockaddr));
    if(rc < 0){
        perror("Connect\n");
        exit(EXIT_FAILURE);
    }

    struct tcp_packet tcp_msg;
    tcp_msg.type = 0;
    strcpy(tcp_msg.content, argv[1]);
    int len = send(tcp_sockfd, (char*)&tcp_msg, sizeof(struct tcp_packet),0);
    if(len == -1){
        perror("TCP send on first message\n");
        exit(EXIT_FAILURE); 
    }
    

    // avem nevoie de un set de file descriptori pentru a putea
    // da comenzi serverului in timp ce suntem conectati
    
    fd_set read_fds_set, fds;
    FD_ZERO(&read_fds_set);
	FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
	FD_SET(tcp_sockfd, &fds);
    int fd_max = tcp_sockfd;
    
    char buf[BUFFLEN];
    

    while(1){

        read_fds_set = fds;
        // asteptam input pe oricare dintre socketi
        if(select(fd_max + 1, &read_fds_set, NULL, NULL, NULL) == -1){
            perror("Select\n");
            exit(EXIT_FAILURE);
        }
            // exista input de la stdin
        if(FD_ISSET (STDIN_FILENO,&read_fds_set)){
            // char buf[BUFFLEN] = {0};
            std::string buf;
            getline(std::cin,buf);
         
            if(buf.find("exit") != std::string::npos){
                struct tcp_packet exit_msg;
                exit_msg.type = 1;
                strcpy(exit_msg.content, argv[1]);
                int len = sendto(tcp_sockfd, (char*)&exit_msg, sizeof(struct tcp_packet),0, (const sockaddr*)&sv_addr, sizeof(sv_addr));
                if(len < 0){
                    perror("Exit message send\n");
                    exit(EXIT_FAILURE);
                }
                break;
            } else if(buf.find("subscribe") != std::string::npos && !(buf.find("unsubscribe") != std::string::npos)){
                std::string topic;
                std::string subscription_type;
                std::stringstream words(buf);
                struct tcp_packet subscribe_to_topic = {0};
                subscribe_to_topic.type = 2;
                int i = 0;
                words >> topic;
                words >> topic;
                words >> subscription_type;
                const char* str = topic.c_str();
                strcpy(subscribe_to_topic.content, argv[1]);
                strcat(subscribe_to_topic.content," ");
                strcat(subscribe_to_topic.content,str);
                strcat(subscribe_to_topic.content," ");
                strcat(subscribe_to_topic.content,subscription_type.c_str());
                int n = send(tcp_sockfd, (char*)&subscribe_to_topic, sizeof(struct tcp_packet),0);
                if(n < 0){
                    perror("subscription failed\n");
                    exit(EXIT_FAILURE);
                }
                printf("Subscribed to topic.\n");
            } else if(buf.find("unsubscribe") != std::string::npos){
                std::string word;
                std::stringstream words(buf);
                struct tcp_packet subscribe_to_topic = {0};
                subscribe_to_topic.type = 3;
                int i = 0;
                while(words >> word){
                    if(i == 1) 
                        break;
                }
                const char* str = word.c_str();
                strcpy(subscribe_to_topic.content, argv[1]);
                strcat(subscribe_to_topic.content," ");
                strcat(subscribe_to_topic.content,str);
                int n = send(tcp_sockfd, (char*)&subscribe_to_topic, sizeof(struct tcp_packet),0);
                if(n < 0){
                    perror("unsubscription failed\n");
                    exit(EXIT_FAILURE);
                }
                printf("Unsubscribed from topic.\n");
            }
            
        }
        if (FD_ISSET(tcp_sockfd, &read_fds_set)) {
			// memset(buf, 0, BUFFLEN);
            int n = recv(tcp_sockfd, buf, sizeof(struct udp_packet)+13, 0);
            if (n < 0) {
                perror("RECV \n");
                exit(EXIT_FAILURE);
            }
            
            struct udp_packet date = {0};
            memcpy(&date,buf,sizeof(struct udp_packet));
            int msg = date.tip_date;
            std::string tip_date;
            if(msg == 0){
                tip_date = "INT";
            } else if(msg == 1){
                tip_date = "SHORT_REAL";
            } else if(msg == 2){
                tip_date = "FLOAT";
            } else if(msg == 3){
                tip_date = "STRING";
            } else{
                printf("Server closing connection...\n");
                exit(0);
            }
                printf("%s - %s - %s\n", date.topic, tip_date.c_str(), date.content);
        }
		
    }
        

    
    close(tcp_sockfd);
    
    return 0;
}