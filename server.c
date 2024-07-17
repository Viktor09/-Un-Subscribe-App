#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <netinet/tcp.h>
#include <math.h>

#define MAX 1552
#define SA struct sockaddr 
#define MAX_TOPIC_LEN 50
#define MAX_TOPICS 101
#define CLIENTS 32
#define MAX_ID_LEN 11
#define SA struct sockaddr 
#define MAX_CLIENTS -1
#define MAX_BUFF 2000

struct id_and_sockfd_client{
    char id[MAX_ID_LEN];
    int sockfd;
    char **topic;
    int count_topic;
    int active;
};

int send_all(int sockfd, char *buf, int len){
	int rc;
	while(len > 0){
		rc = send(sockfd, buf, len, 0);
		if(rc <= 0){
			return rc;
		}
		buf += rc;
		len -= rc;
	}
	return rc;
}


void error(const char *msg){
    perror(msg);
    exit(1);
}

int isMatch(char **topic_plus_star, int size_topic_plus_star, char **topic, int size_topic) {
    int dp[size_topic_plus_star + 1][size_topic + 1];
    bzero(dp, sizeof(dp));
    
    dp[0][0] = 1;
    for (int i = 1; i <= size_topic_plus_star && strcmp(topic_plus_star[i - 1], "*") == 0; ++i)
        dp[i][0] = 1;

    for (int i = 1; i <= size_topic_plus_star; ++i) {
        for (int j = 1; j <= size_topic; ++j) {
            if (strcmp(topic_plus_star[i - 1], "*") == 0) {
                dp[i][j] = dp[i - 1][j - 1] || dp[i][j - 1];
            } else if (strcmp(topic_plus_star[i - 1], "+") == 0 || strcmp(topic_plus_star[i - 1], topic[j - 1]) == 0) {
                dp[i][j] = dp[i - 1][j - 1];
            }
        }
    }

    return dp[size_topic_plus_star][size_topic];
}



int main(int argc, char **argv) 
{   
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd_tcp, sockfd_udp; 
    struct sockaddr_in servaddr_tcp, servaddr_udp;
    struct sockaddr_in servaddr_client;
    fd_set readfds;
    fd_set master; 
    int fdmax;

    char buff[MAX_BUFF]; 
    bzero(buff, MAX_BUFF);    
    
    int sockid_counter = 0;
    struct id_and_sockfd_client *idsock = calloc(CLIENTS, sizeof(struct id_and_sockfd_client));
    for(int i = 0; i < CLIENTS; i++){
        bzero(idsock[i].id, 11);
        idsock[i].active = 0;
        idsock[i].count_topic = 0;
        idsock[i].topic = calloc(MAX_TOPICS, sizeof(char *));
        for(int j = 0; j < MAX_TOPICS; j++){
            idsock[i].topic[j] = calloc(MAX_TOPIC_LEN, sizeof(char));
            if (idsock[i].topic[j] == NULL) {
                error("Calloc failed for topic");
            }
            bzero(idsock[i].topic[j], MAX_TOPIC_LEN);
        }
    }

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd_tcp < 0) { 
        error("socket creation failed");
    } 

    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd_udp < 0) { 
        error("socket creation failed");
    } 

    bzero(&servaddr_tcp, sizeof(servaddr_tcp));  
    servaddr_tcp.sin_family = AF_INET; 
    servaddr_tcp.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr_tcp.sin_port = htons(atoi(argv[1])); 

    bzero(&servaddr_udp, sizeof(servaddr_udp));
    servaddr_udp.sin_family = AF_INET;
    servaddr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr_udp.sin_port = htons(atoi(argv[1]));

    FD_ZERO(&master);
    FD_SET(sockfd_tcp, &master);
    FD_SET(sockfd_udp, &master);
    FD_SET(0, &master);
    
    const int enable = 1;
    if (setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("error reuse port");
        return -1;
    }

    if (setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
        perror("error naggle");
        return -1;
    }

    if ((bind(sockfd_tcp, (SA*)&servaddr_tcp, sizeof(servaddr_tcp))) < 0) { 
        error("socket bind failed...");
    }
    if ((bind(sockfd_udp, (SA*)&servaddr_udp, sizeof(servaddr_udp))) < 0) { 
        error("socket bind failed...");
    } 

    if ((listen(sockfd_tcp, -1)) != 0) { 
        error("listen failed");
    }

    fdmax = sockfd_udp;
    int ok = 0; 

    for (;;) {
        readfds = master;
        if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4); 
        }
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == 0) {
                    bzero(buff, MAX); 
                    fgets(buff, MAX - 1, stdin);
                    buff[strcspn(buff, "\n")] = '\0'; 

                    if (strncmp(buff, "exit", 4) == 0) {
                        ok = 1;
                        break;
                    }
                } 
                else if (i == sockfd_tcp) {
                    int flag = 1;
                    socklen_t addrlen = sizeof(servaddr_client);
                    int sockfd_client = accept(sockfd_tcp, (struct sockaddr *)&servaddr_client, &addrlen);
                    if (sockfd_client < 0) {
                        error("server accept failed");
                    }
                    if(setsockopt(sockfd_client, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0){
		                error("error naggle");
	                }
                    if(recv(sockfd_client, buff, MAX-1, 0) < 0){
                        error("error recv tcp");
                    }

                    int ok = 0;
                    int j;
                    for(j = 0; j < sockid_counter; j++){
                        
                        int i = 0;
                        int cnt = 0;
                        while(buff[i] != ' '){
                            if(buff[i] == idsock[j].id[i]){
                                cnt++;
                            }
                            i++;
                        }
                        if(cnt == i && idsock[j].active == 1){
                            ok = 1;
                            break;
                        }
                    }

                    char newbuff[MAX];
                    bzero(newbuff, MAX);
                    int offset = 0;
                    
                    if(ok == 1){
                        strcat(newbuff, "Client ");
                        strcat(newbuff, idsock[j].id);
                        strcat(newbuff, " already connected.");
                        
                    }else{
                        if(sockid_counter == CLIENTS){
                            struct id_and_sockfd_client *idsock = realloc(idsock, sockid_counter + CLIENTS);
                            for(int i = sockid_counter; i < CLIENTS + sockid_counter; i++){
                                bzero(idsock[i].id, 11);
                                idsock[i].active = 0;
                                idsock[i].count_topic = 0;
                                idsock[i].topic = calloc(MAX_TOPICS, sizeof(char *));
                                for(int j = 0; j < MAX_TOPICS; j++){
                                    idsock[i].topic[j] = calloc(MAX_TOPIC_LEN, sizeof(char));
                                    if (idsock[i].topic[j] == NULL) {
                                        error("Calloc failed for topic");
                                    }
                                    bzero(idsock[i].topic[j], MAX_TOPIC_LEN);
                                }
                            }
                        }
                        FD_SET(sockfd_client, &master);
                        if (sockfd_client > fdmax) {
                            fdmax = sockfd_client;
                        }
                        int ok1 = 0;
                        int j;
                        for(j = 0; j < sockid_counter; j++){
                            
                            int i = 0;
                            int cnt = 0;
                            while(buff[i] != ' '){
                                if(buff[i] == idsock[j].id[i]){
                                    cnt++;
                                }
                                i++;
                            }
                            if(cnt == i && idsock[j].active == 0){
                                ok1 = 1;
                                break;
                            }
                        }

                        if(ok1 == 1){
                            idsock[j].active = 1;
                            idsock[j].sockfd = sockfd_client;

                            strcat(newbuff, "New client ");
                            offset += strlen(newbuff);

                            int i = 0;
                            while(buff[i] != ' '){
                                newbuff[offset] = buff[i];
                                idsock[j].id[i] = buff[i];
                                i++;
                                offset++;
                                
                            }
                        
                            strcat(newbuff, " connected from");
                            offset += 15;
        
                            while(buff[i] != '\0'){
                                newbuff[offset] = buff[i];
                                i++;
                                offset++;
                            }
                            newbuff[offset] = '\0'; 

                        }else{
                            idsock[sockid_counter].active = 1;
                            idsock[sockid_counter].sockfd = sockfd_client;
                        
                            strcat(newbuff, "New client ");
                            offset += strlen(newbuff);

                            int i = 0;
                            while(buff[i] != ' '){
                                newbuff[offset] = buff[i];
                                idsock[sockid_counter].id[i] = buff[i];
                                i++;
                                offset++;
                                
                            }
                        
                            strcat(newbuff, " connected from");
                            offset += 15;
        
                            while(buff[i] != '\0'){
                                newbuff[offset] = buff[i];
                                i++;
                                offset++;
                            }
                            newbuff[offset] = '\0'; 
                            
                            sockid_counter++;
                        }

                        
                    }

                    printf("%s\n", newbuff); 
                    if(ok == 1){
                        close(sockfd_client);
                    }

                } else if (i == sockfd_udp){
                    
                    socklen_t addrlen = sizeof(servaddr_udp);
                    bzero(buff, MAX_BUFF);
                    if (recvfrom(sockfd_udp, buff, MAX_BUFF, 0, (struct sockaddr*) &servaddr_udp, &addrlen) < 0) {
                        error("nothing received");
                    }
                    
                    char topic[50];
                    bzero(topic, 50);
                    memcpy(topic, buff, 50);
      
                    char **vector_topic = calloc(10, sizeof(char *));
                    for(int i = 0; i < 10; i++) {
                        vector_topic[i] = calloc(30, sizeof(char));
                    }
                                    
                    const char s[2] = "/";
                    char *auxi = strdup(topic);
                    char *token = strtok(auxi, s);

                    int size_topic = 0;
                    while( token != NULL ) {
                        if(token[strlen(token) - 1] == '\n'){
                            token[strlen(token) - 1] = '\0';
                        }
                        vector_topic[size_topic] = token;
                        size_topic++;
                        token = strtok(NULL, s);
                    }

                    int j = 0;
                    for (j = 0; j < sockid_counter; j++) {
                        if(idsock[j].active == 1){
                            for (int k = 0; k < idsock[j].count_topic; k++) {
                                int k1 = 0;
                                int ok1 = 0;
                                 while(idsock[j].topic[k][k1] != '\0'){
                                    if(idsock[j].topic[k][k1] == '+' || idsock[j].topic[k][k1] == '*'){
                                        ok1 = 1;
                                        break;
                                    }
                                    k1++;
                                }
                                    
                                if(ok1 == 1){
                                    char **vector_topic_plus_star = calloc(10, sizeof(char *));
                                    for(int i = 0; i < 10; i++) {
                                    vector_topic_plus_star[i] = calloc(30, sizeof(char));
                                }
                                        
                                char *aux_topic = strdup(idsock[j].topic[k]);
                                char *token_aux = strtok(aux_topic, s);
                                        
                                int size_topic_plus_star = 0;
                                while( token_aux != NULL ) {
                                            
                                    if(token_aux[strlen(token_aux) - 1] == '\n'){
                                        token_aux[strlen(token_aux) - 1] = '\0';
                                    }

                                    vector_topic_plus_star[size_topic_plus_star] = token_aux;
                                    size_topic_plus_star++;
                                    token_aux = strtok(NULL, s);
                                }
                                        
                                int okay = isMatch(vector_topic_plus_star, size_topic_plus_star, vector_topic, size_topic);
                                if(okay == 1){
                                    int len_ip_udp = strlen(inet_ntoa(servaddr_udp.sin_addr)) + 1;
                                    char *ip_udp = calloc(1, len_ip_udp);
                                    memcpy(ip_udp, inet_ntoa(servaddr_udp.sin_addr), strlen(inet_ntoa(servaddr_udp.sin_addr)));
                                    if(send(idsock[j].sockfd, ip_udp, len_ip_udp, 0) < 0){
                                        error("send error");
                                    }
                                    char ok_ip[5];
                                    bzero(ok_ip, 5);
                                    if(recv(idsock[j].sockfd, ok_ip, 5, 0) < 0){
                                        error("recv error");
                                    }
                                            
                                    if(strcmp("okip", ok_ip) == 0){
                                        char port[6];
                                        bzero(port, 6);
                                        sprintf(port, "%d",ntohs(servaddr_udp.sin_port));
                                        if(send(idsock[j].sockfd, port, 6, 0) < 0){
                                            error("send error");
                                        }
                                        char ok_port[7];
                                        bzero(ok_port, 7);
                                        if(recv(idsock[j].sockfd, ok_port, 7, 0) < 0){
                                            error("recv error");
                                        }
                                        if(strcmp(ok_port, "okport") == 0){
                                            int rc = send_all(idsock[j].sockfd, buff, MAX_BUFF);
                                            if(rc <= 0){
                                                error("send error");
                                            }
                                        }
                                    }
                                            
                                }

                                }else{
                                    if (strncmp(idsock[j].topic[k], topic, strlen(topic)-1) == 0) {
                                        int len_ip_udp = strlen(inet_ntoa(servaddr_udp.sin_addr)) + 1;
                                        char *ip_udp = calloc(1, len_ip_udp);
                                        memcpy(ip_udp, inet_ntoa(servaddr_udp.sin_addr), strlen(inet_ntoa(servaddr_udp.sin_addr)));
                                        if(send(idsock[j].sockfd, ip_udp, len_ip_udp, 0) < 0){
                                            error("send error");
                                        }
                                        char ok_ip[5];
                                        bzero(ok_ip, 5);
                                        if(recv(idsock[j].sockfd, ok_ip, 5, 0) < 0){
                                            error("recv error");
                                        }
                                            
                                        if(strcmp("okip", ok_ip) == 0){
                                            char port[6];
                                            bzero(port, 6);
                                            sprintf(port, "%d",ntohs(servaddr_udp.sin_port));
                                            if(send(idsock[j].sockfd, port, 6, 0) < 0){
                                                error("send error");
                                            }
                                            char ok_port[7];
                                            bzero(ok_port, 7);
                                            if(recv(idsock[j].sockfd, ok_port, 7, 0) < 0){
                                                error("recv error");
                                            }
                                                
                                            if(strcmp(ok_port, "okport") == 0){
                                                 int rc = send_all(idsock[j].sockfd, buff, MAX_BUFF);
                                                 if(rc <= 0){
                                                     error("send error");
                                                 }
                                                   
                                                
                                            }

                                        }
                                    } 
                                }
                            }
                        }
                    }
                }
                else {
                    bzero(buff, MAX_BUFF); 
                    int nbytes = recv(i, buff, MAX_BUFF, 0);
                    
                    if(nbytes < 0){
                        error("error recv");
                    }

                    if (nbytes == 0) {
                        int oki = 0;
                        int j = 0;
                        for(j = 0; j < sockid_counter; j++){
                            if(idsock[j].active == 1 && idsock[j].sockfd == i){
                                idsock[j].active = 0;
                                oki = 1;
                                break;
                            }
                        }
                        if(oki == 1){
                            printf("Client %s disconnected.\n", idsock[j].id);
                        }

                        idsock[j].active = 0;
                        FD_CLR(i, &master);
                        
                        close(i);
                    } else {
                        
                        int j = 0;
                        if(strncmp(buff, "subscribe", 9) == 0){    
                            for(j = 0; j < sockid_counter; j++){
                                if(strncmp(idsock[j].id, buff + 10, strlen(idsock[j].id)) == 0 && idsock[j].active == 1){
                                    
                                    if(idsock[j].count_topic == MAX_TOPICS){
                                        idsock[j].topic = realloc(idsock[j].topic , MAX_TOPICS + idsock[j].count_topic);
                                        for(int k = idsock[j].count_topic; k < idsock[j].count_topic + MAX_TOPICS; k++){
                                            idsock[j].topic[k] = calloc(MAX_TOPIC_LEN, sizeof(char));
                                            if (idsock[j].topic[k] == NULL) {
                                                error("Calloc failed for topic");
                                            }
                                            bzero(idsock[j].topic[k], MAX_TOPIC_LEN);
                                        }
                                    }

                                    int okay = 0;
                                    for(int k = 0; k < idsock[j].count_topic; k++){
                                        if(strcmp(buff + 9 +strlen(idsock[j].id) + 2, idsock[j].topic[k]) == 0){
                                            okay = 1;
                                            break;
                                        }
                                    }
                                    if(okay == 0){
                                        memcpy(idsock[j].topic[idsock[j].count_topic], buff + 9 +strlen(idsock[j].id) + 2, 50);
                                        idsock[j].count_topic++;
                                    }

                                    
                                }
                            }
                        } else if(strncmp(buff, "unsubscribe", 11) == 0){
                            
                            char topic[74];
                            strcpy(topic, buff);
                            
                            for (j = 0; j < sockid_counter; j++){
                                if(strncmp(idsock[j].id, buff + 12, strlen(idsock[j].id)) == 0 && idsock[j].active == 1){
                                    for (int k = 0; k < idsock[j].count_topic; k++) {
                                        if (strncmp(idsock[j].topic[k], topic + 11 + 2 + strlen(idsock[j].id), strlen(idsock[j].topic[k])) == 0) {
                                            for (int l = k; l < idsock[j].count_topic - 1; l++) {
                                                strcpy(idsock[j].topic[l], idsock[j].topic[l + 1]);
                                            }
                                            idsock[j].count_topic--;
                                            break; 
                                        }
                                    }
                                }
                               
                            }
                        }
                    }
                }
            }
        }
        if (ok == 1) {
            break;
        } 
    }

    for (int i = 2; i <= fdmax; ++i) {
        if (FD_ISSET(i, &master)) {
            close(i);
        }
    }
}