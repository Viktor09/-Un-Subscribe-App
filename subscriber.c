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

#define MAX 2000
#define MAX_INPUT 100
#define SA struct sockaddr
#define MAX_SUB 72
#define MAX_UNSUB 74
#define MAX_TOPIC_LEN 50
#define OK_IP_LEN  5
#define OK_PORT_LEN 7
#define IP_LEN 32
#define PORT_LEN 6
#define STRING_LEN 1501

int recv_all(int sockfd, char *buf, int len){
	int rc;
	while(len > 0){
		rc = recv(sockfd, buf, len, 0);
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

int main(int argc, char **argv)
{

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd_server;
	struct sockaddr_in servaddr;
	fd_set master, readfds;

	
    char newbuff[MAX];
	int offset = 0;
	bzero(newbuff, MAX);

	memcpy(newbuff, argv[1], strlen(argv[1]));
	offset += strlen(argv[1]);
	newbuff[offset] = ' ';
	offset++;

	memcpy(newbuff + offset, argv[2], strlen(argv[2]));
	offset += strlen(argv[2]);
	newbuff[offset] = ':';
	offset++;

	memcpy(newbuff + offset, argv[3], strlen(argv[3]));
	offset += strlen(argv[3]);
	newbuff[offset] = '.';
	offset++;
	
	newbuff[offset] = '\0';
	offset++;
	


	sockfd_server = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd_server < 0) {
		error("socket creation failed"); 
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_aton(argv[2], &servaddr.sin_addr);
	servaddr.sin_port = htons(atoi(argv[3]));

	if(connect(sockfd_server, (SA*)&servaddr, sizeof(servaddr)) < 0){
		error("couldn't connect");
	}

	int flag = 1;
	if(setsockopt(sockfd_server, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0){
		error("error naggle");
	}
	
	if(send(sockfd_server, newbuff, offset, 0) < 0){
		error("send error");
	} 


	FD_ZERO(&master);
	FD_SET(sockfd_server, &master);
	FD_SET(0, &master);

	for(; ;){
		readfds = master;
		if (select(sockfd_server + 1, &readfds, NULL, NULL, NULL) == -1){
            perror("select");
            exit(4); 
        }

		if(FD_ISSET(0, &readfds)){

			char buff[MAX_INPUT]; 
			bzero(buff, MAX_INPUT);
            fgets(buff, MAX_INPUT - 1, stdin);
                    
        	if(strncmp(buff, "exit", 4) == 0){
                break;
            }
			else{
			
				if(strncmp(buff, "subscribe", 9) == 0){
					char newbuffmsj[MAX_SUB];
					bzero(newbuffmsj, MAX_SUB);
					
					strcat(newbuffmsj, "subscribe^");
					strcat(newbuffmsj, argv[1]);
					

					strcat(newbuffmsj, "^");
					strcat(newbuffmsj, buff + 10);
					
					if(send(sockfd_server, newbuffmsj, strlen(newbuffmsj) + 1, 0) < 0){
						error("send error");
					}
					
					printf("Subscribed to topic %s", buff + 10);
				} else if(strncmp(buff, "unsubscribe", 11) == 0){
					char newbuffmsj[MAX_UNSUB];
					bzero(newbuffmsj, MAX_UNSUB);

					strcat(newbuffmsj, "unsubscribe^");
					strcat(newbuffmsj, argv[1]);
					

					strcat(newbuffmsj, "^");
					strcat(newbuffmsj, buff + 12);
					
					if(send(sockfd_server, newbuffmsj, strlen(newbuffmsj) + 1, 0) < 0){
						error("send error");
					}
					
					printf("Unsubscribed from topic %s", buff + 12);
				}
				
			}
		}
		if(FD_ISSET(sockfd_server, &readfds)){
			
			int rc;
			char ip_udp[IP_LEN];
			bzero(ip_udp, IP_LEN);

			if(recv(sockfd_server, ip_udp, IP_LEN, 0) < 0){
				error("error recv");
			}
			char ok_ip[OK_IP_LEN] = "okip\0";
			if(send(sockfd_server, ok_ip, OK_IP_LEN, 0) < 0){
				error("error send");
			}
			char port_udp[PORT_LEN];
			bzero(port_udp, PORT_LEN);
	
			if(recv(sockfd_server, port_udp, PORT_LEN, 0) < 0){
				error("error recv");
			}
			
			char ok_port[OK_PORT_LEN] = "okport\0";
			if(send(sockfd_server, ok_port, OK_PORT_LEN, 0) < 0){
				error("error send");
			}

			char buff[MAX]; 
			bzero(buff, MAX);
			
			rc = recv_all(sockfd_server, buff, MAX);
			
			if (rc < 0) {
				error("recv error"); 
			}
			else if(rc == 0){
				break;
			} 
		
			char topic[MAX_TOPIC_LEN];
            bzero(topic, MAX_TOPIC_LEN);
            memcpy(topic, buff, MAX_TOPIC_LEN);
                    
            uint8_t *type_ptr = (uint8_t *)(buff + 50);
            uint8_t type = *type_ptr;
                   
            if(type == 0){
                long long int_num;
                uint8_t *byte_semn_ptr = (uint8_t *)(buff + 51);
                uint8_t byte_semn = *byte_semn_ptr;

                uint32_t *payload_ptr = (uint32_t *)(buff + 52);
                uint32_t payload = *payload_ptr;
                int_num = (int32_t)ntohl(payload);

                if (byte_semn == 1) {
                    int_num = -(int32_t)ntohl(payload);
                }
                printf("%s:%s - %s - INT - %lld\n", ip_udp, port_udp, topic, int_num);
            } else if(type == 1){
                uint16_t *short_real_ptr = (uint16_t *)(buff + 51);
                uint16_t short_real = ntohs(*short_real_ptr);
                double real = short_real / 100.0;
                printf("%s:%s - %s - SHORT_REAL - %.2f\n", ip_udp, port_udp,topic, real);
            } else if(type == 2){
                uint8_t *byte_semn_ptr = (uint8_t *)(buff + 51);
                uint8_t byte_semn = *byte_semn_ptr;

                uint32_t *big_real_ptr = (uint32_t *)(buff + 51 + sizeof(uint8_t));
                uint32_t big_real = ntohl(*big_real_ptr);

                uint8_t *divider_ptr = (uint8_t *)(buff + 51 + sizeof(uint32_t) + sizeof(uint8_t));
                uint8_t divider = *divider_ptr;

                double real_value = big_real * pow(10, -divider);

                if (byte_semn == 1) {
                    real_value *= -1;
                }
                printf("%s:%s - %s - FLOAT - %f\n",ip_udp, port_udp, topic, real_value);
            } else if(type == 3){
                char payload[STRING_LEN];
                bzero(payload, STRING_LEN);
                size_t payload_len = strlen(buff + 51);
                memcpy(payload, buff + 51, payload_len);
                payload[payload_len] = '\0'; 
                printf("%s:%s - %s - STRING - %s\n", ip_udp, port_udp, topic, payload);
            }

		}

	}

	close(sockfd_server);
}