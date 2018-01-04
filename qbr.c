#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <pthread.h>
#include <json/json.h>

#define JSON_KEY_ACK "acknowledgement"
#define JSON_VALUE_OK 1
#define JSON_REQUEST_STREAMING 1
#define JSON_REQUEST_DISCONNECT 9
#define BUFFER_SIZE 8192

struct mypara
{
    int clientSocket;
    const char *imei;
};

void sendAcknowledgement(int clientSocket, int code)
{
    char temp_buff[1024];
    json_object *jobj = json_object_new_object();
    json_object *jval = json_object_new_int(code);
    json_object_object_add(jobj, JSON_KEY_ACK, jval);
    if (strcpy(temp_buff, json_object_to_json_string(jobj)) == NULL) {
        printf("ERROR copy json object\n");
     }
    // send an acknowledgement
    if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1) {
        printf("ERROR write json object\n");
     }
}

void write_log(char *log)  
{  
    FILE *fp;  
    char buffer[200];  
    memset(buffer,0,sizeof(buffer));  
    fp=fopen("deepeye.log","a+");  
    if( NULL != fp ){ 
        struct tm *t;
        time_t tt;
        time_t ts;
        time(&tt);
        t=localtime(&tt);
        sprintf(buffer,"%02d:%02d:%02d %s\n",t->tm_hour,t->tm_min,t->tm_sec,log);  
        fputs(buffer, fp);  
        fclose(fp);  
    }  
    else{  
        printf("Cant't Open log file\n");  
    }  
}  

void *procthread(void *arg)
{
     struct mypara *recv_para;
     recv_para = (struct mypara *)arg;
     const char *imei = (*recv_para).imei;
     char *darknet;
     darknet=malloc(300);
     sprintf(darknet,"./darknet detect cfg/yolo.cfg yolo.weights %s",imei);
     system(darknet);
}

void *recvthread(void *arg)
{
     struct mypara *recv_para;
     recv_para = (struct mypara *)arg;
     int _client = (*recv_para).clientSocket;// *((int *)client);
     bool run = true;
     bool start = true;
     char small_buff[1024];
     int num = 0;
     int num_bytes;
     int bytes = 0;
     const char *gps;
     char* n;
     n=(char *)malloc(200);
     const char *imei = (*recv_para).imei;
     while(run){
         if((num_bytes = read(_client, small_buff, 1024))<= 0){
             printf("No byte received\n");
             run = false;
             break;
         }
         struct json_object *jobj = json_tokener_parse(small_buff);
	     int request = -1;
         json_object_object_foreach(jobj, key, val){
             if(strcmp(key, "request")==0){
                 request = json_object_get_int(val);
             }
             if(strcmp(key, "GPS")==0){
                 gps = json_object_get_string(val);
             }
         }
         char *log;
         log=(char *)malloc(200);
         sprintf(log, "%s_%d: %s", imei, num, gps);
         write_log(log);
         switch (request) {
             case JSON_REQUEST_STREAMING:
             { sendAcknowledgement(_client, JSON_VALUE_OK);
                 json_object_object_foreach(jobj, key, val) {
                     if (strcmp(key, "bytes") == 0) {
                         bytes = json_object_get_int(val);
                     }
             }
             sprintf(n, "./images/%s_%d.jpg", imei, num);
             FILE *fp = fopen(n, "w");
             char buffer[BUFFER_SIZE];
             int offset = 0, length = 0;
             while((length = read(_client, buffer, BUFFER_SIZE))>0)
             {
                 int write_length = fwrite(buffer, sizeof(char), length, fp);
                 offset += length;
                 if (offset >= bytes) {
                     break;
                 }
                 bzero(buffer, BUFFER_SIZE);  

             }
             fclose(fp);
             memset(n,0,200);
             num++;
             sendAcknowledgement(_client, JSON_VALUE_OK);}
             break;

             case JSON_REQUEST_DISCONNECT:
             { run = false;}
             break;

             default:
             { printf("No request received\n");
             run = false;}
             break;
        }
     }
     close(_client);
     struct tm *t;  
     time_t tt;  
     time_t ts;          
     time(&tt);  
     t = localtime(&tt);
     char *m_qbr;
     m_qbr=malloc(200);
     sprintf(m_qbr,"./delete_qbr.sh %s %4d%02d%02d_%02d:%02d:%02d_%s",imei,t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,imei);
     system(m_qbr);
  //   pthread_exit(0);
     return 0;
}

int main(int argc, char **argv)
{
   static const int PORT_NUM = 1050;
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
           pthread_t _thread1;
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
           if((pthread_create(&_thread1,NULL,procthread,&(para)))!= 0){
                    printf("Can't create thread\n");
	   	            return 0;
           }
      //     pthread_join(_thread, NULL);
      //     pthread_join(_thread1, NULL);
	}
}
