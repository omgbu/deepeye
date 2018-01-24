#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h> 
#include <sys/select.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <json/json.h>

#define JSON_BYTE "bytes"
#define JSON_VALUE_OK 1
#define JSON_REQUEST_STREAMING 1
#define JSON_REQUEST_DISCONNECT 9
#define BUFFER_SIZE 8192

struct mypara
{
    int clientSocket;
    const char *imei;
};

void sendBytes(int clientSocket, int length)
{
    char temp_buff[1024];
    json_object *jobj = json_object_new_object();
    json_object *jval = json_object_new_int(length);
    json_object_object_add(jobj, JSON_BYTE, jval);
    if (strcpy(temp_buff, json_object_to_json_string(jobj)) == NULL) {
        printf("ERROR copy json object\n");
     }
    // send image bytes
    if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1) {
        printf("ERROR write json object\n");
     }
}

void recvok(char* small_buff, int json_ok)
{
	struct json_object *jobj = json_tokener_parse(small_buff);
    	json_object_object_foreach(jobj, key, val) {
        	if (strcmp(key, "acknowledgement") == 0) {
           		json_ok  = json_object_get_int(val);
        	}
    	}
}

void *recvthread(void *arg)
{
     struct mypara *recv_para;
     recv_para = (struct mypara *)arg;
     int _client = (*recv_para).clientSocket;// *((int *)client);
     bool run = true;
     char small_buff[1024];
     int T = 0;
     int i = 0;
     int N = 10000000;
     int num = 0;
     int num_bytes;
     int json_ok = 0;
     char* f;
     f = (char *)malloc(200);
     const char *imei = (*recv_para).imei;
     fd_set readfd;
     sleep(10);
     while(run){
	sprintf(f, "./img/%s_%d.jpg", imei, num);
	if((access(f,0))!= -1){
		i = 0;
		struct stat statbuf;  
    		stat(f,&statbuf);  
    		int size=statbuf.st_size; 
		sendBytes(_client, size);
                printf("SEND: %d\n",num);
		struct timeval timeout;
         	timeout.tv_sec= 6;
                timeout.tv_usec= 0;
                FD_ZERO(&readfd);
                FD_SET(_client, &readfd);
                switch (select(_client + 1, &readfd, NULL, NULL, &timeout)) {
             		case -1:
             		{ run = false;}
             		break;

             		case 0:
             		{ run = false;}
             		break;

             		default:
			if(FD_ISSET(_client, &readfd)){
        	            	if((num_bytes = read(_client, small_buff, 1024))<= 0){
             		        	printf("No ok received\n");
             		       	 	run = false;
             		        	break;
         	            	} 
			}
		 }  
		 recvok(small_buff, json_ok);
	         FILE *fp = fopen(f, "rb");
                 char buffer[BUFFER_SIZE];
		 int file_block_length = 0;
  		 while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0){
			if (send(_client, buffer, file_block_length, 0) < 0){
				printf("Send File Failed!\n");  
                                break;  
			}
		        bzero(buffer, sizeof(buffer));
		 }   
	         fclose(fp);
	         struct timeval timeout1;
	         timeout1.tv_sec= 6;
	         timeout1.tv_usec= 0;
	         FD_ZERO(&readfd);
	         FD_SET(_client, &readfd);
		 switch (select(_client + 1, &readfd, NULL, NULL, &timeout1)) {
             		case -1:
             		{ run = false;}
             		break;

             		case 0:
             		{ run = false;}
             		break;

             		default:
			if(FD_ISSET(_client, &readfd)){
        	        	if((num_bytes = read(_client, small_buff, 1024))<= 0){
             		        	printf("No ok received\n");
             		        	run = false;
             		        	break;
         	            	}
			}
		 }
		 recvok(small_buff, json_ok);
                 memset(f,0,200);
                 num++;
	}else{
		 i++;
	}
        if(i>N){
		run = false;
		break;
	}
    }
    struct tm *t;  
    time_t tt;  
    time_t ts;          
    time(&tt);  
    t = localtime(&tt);
    char *m_qbr;
    m_qbr=malloc(200);
    sprintf(m_qbr,"./mv_qbr.sh %s %4d%02d%02d_%02d:%02d:%02d_%s",imei,t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,imei);
    system(m_qbr);
    close(_client);
  //   pthread_exit(0);
    return 0;
}

int main(int argc, char **argv)
{
   static const int PORT_NUM = 1051;
   struct sockaddr_in6 serv_addr;
   int mSocketFD = 0;
   mSocketFD = socket(AF_INET6, SOCK_STREAM, 0);
   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin6_family = AF_INET6;
   serv_addr.sin6_addr = in6addr_any;
   serv_addr.sin6_port = htons(PORT_NUM);
   if (bind(mSocketFD, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         printf("ERROR on binding"); }
   listen(mSocketFD, 1);
   while(1)
        {
           socklen_t client;
           struct sockaddr_in6 cli_addr;
           struct mypara para;
           pthread_t _thread;
           char small_buff[1024];
           int num_bytes;
           client = sizeof(cli_addr);
           para.clientSocket = accept(mSocketFD, (struct sockaddr *)&cli_addr, &client);
           if (para.clientSocket < 0) {
           printf("ERROR on accept"); }
           if((num_bytes = read(para.clientSocket, small_buff, 1024))<= 0){
                    printf("No bytes received\n");
                    return 0;
           }
           struct json_object *jobj = json_tokener_parse(small_buff);
           json_object_object_foreach(jobj, key, val){
                if(strcmp(key, "imei")==0){
                    para.imei = json_object_get_string(val);
                }
           }
           if((pthread_create(&_thread,NULL,recvthread,&(para)))!= 0){
                    printf("Can't create thread\n");
	    return 0;
           }
      //     pthread_join(_thread, NULL);
   }
}
