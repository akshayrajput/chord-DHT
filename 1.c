#ifndef _ALL_STD_H_
#define _ALL_STD_H
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<assert.h>
#include<setjmp.h>
#include<ctype.h>
#include<time.h>
#include<unistd.h>
#define REP(i,a,b) \
	for(int i = a; i <= b; i++)

#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <math.h>
#include <openssl/md5.h>
     
#include "./MyThread.h"


typedef struct f{
	char key[20];
	char value[20];
	int available;
}datanode;

datanode table[100];

char VAL[20];

typedef struct g{
	int id;
	char ip[16];
	int port;
}node;

node fingerTable[33];

node temp;

node predecessor;
node successor;
node mySelf;

char targetIP[16];
int targetPort;

char predBuffer[256];	// find predecessor buffer
char succBuffer[256];	// find successor buffer
char notBuffer[256]; // notify buffer
char getBuffer[256]; // getData buffer
char putBuffer[256]; // putData buffer


/******************Static Functions*****************/
static void putData(char id[21], char value[20]);
static void getData(char id[21]);
static char* getIPAddr();
static int inBetween(int id, int start, int end, int inclusive);
static int compute_md5(const char *key, int len);
/****************End declaration****************/


/*****Normal Functions' Declaration***********/
void server();
void client(char []);
void stabilize();
void notify(node);
void join(node x);
node findSuccessor(node x);
node closestPrecedingNode(node x);
void fixFinger();
void createRing();
void put(char [], char []);
void get (char[], char[]);
/****************End declaration****************/

/*************Static Functions' definition**********/
static int compute_md5(const char *idx, int len)
{   
   // int n;
    MD5_CTX c;
    char ch[100];
    char digest[17];
    digest[16] = '\0';
    char *str;
    //char str[50];
  
   
     sprintf (ch,"%s",idx);
    // int len = strlen(key);
     ch[len] = '\0';
     str=&ch[0];
    
     int length=strlen(str);
     
    MD5_Init(&c);

    while (length > 0)
	{
        if (length > 512)
        {
            MD5_Update(&c, str, 512);
        } 
        else
        {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final((unsigned char *)digest, &c);
    /*char digest[17];
    MD5((const unsigned char *) idx, (unsigned long)len,(unsigned char *)digest );*/
    int temp = 0;
    for(int i = 0; i < 17; i++)
    {
    	temp += abs((int)digest[i]);
    }
    return temp;
}

static void putData(char id[21], char value[20])
{
	int i;
	//printf("In putData key=>[%s] value=>[%s]\n", id, value);fflush(stdout);
	for(i=0; i<100; ++i)
	{
		if(table[i].available)
		{
			strncpy(table[i].key, id, 20);
			strncpy(table[i].value, value, 19);
			table[i].available = 0;
			//printf("Value stored at %d. [%s]=>[%s]\n",mySelf.port,id,value);fflush(stdout);
			return;
		}
  }
  fprintf(stderr,"Not enough space in table\n");
}

static void getData(char id[21])
{
	for(int i=0; i<20; ++i) VAL[i] = '\0';
	printf("Got id = %s",id);

	for(int i=0; i<100; ++i)
	{
		if(strcmp(table[i].key,id) == 0)
		{
			strncpy(VAL, table[i].value, 19);
			//printf("Value returned=>[%s]\n",VAL);fflush(stdout);
			return;
		}
	}
	//printf("REturning null\n");fflush(stdout);
}

static char* getIPAddr()
{
	struct ifaddrs *addrs, *tmp;
	if(getifaddrs(&addrs) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	
	tmp = addrs; 
	
	while (tmp) 
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family==AF_INET && !strcmp(tmp->ifa_name,"eth0"))
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			freeifaddrs(addrs);
			return(inet_ntoa(pAddr->sin_addr));
		}
		
		tmp = tmp->ifa_next;
	}
    
    	freeifaddrs(addrs);
	return 0;
}

static int inBetween(int id, int start, int end, int inclusive)
{
	if( end > start)
	{
		return (((start < id) && (id < end)) || (inclusive && ((id == end))));
	}
	
	else
	{
		return ((start < id) || (id < end) || (inclusive && ((id == end))));
	}
}

/***********End Static Function definition**************/


/***********Normal Function definition**********/
void put(char key[20], char value[20])
{
	node x, y;
	x.ip[0]='\0';
	x.port = 0;
	printf("PUT: Putting key %s and value %s\n",key, value);fflush(stdout);
	x.id = compute_md5((const char *)key, strlen(key));
	 //decimal_of_digest();
	/*for(int i = 0; i < 17; i++)
	{
		if( x.id[i] == '\0' || x.id[i] == ' ') x.id[i] = (char)255; 
	}*/
	//printf("Keys ID is %d\n",x.id);
	//find the node on which we have to put the value
	y = findSuccessor(x);
	//printf("PUT: FIND successor returned id => [%d] ip => [%s] port=>[%d]", y.id, y.ip, y.port);fflush(stdout);
	strcpy(targetIP, y.ip);
	targetPort = y.port;
	for(int i=0; i<255; ++i) putBuffer[i]='\0';
  	sprintf(putBuffer,"putdata %d %s",x.id,value);
  	//printf("PUT: sending data to ip =>[%s] and port =>[%d]\n",targetIP, targetPort);fflush(stdout); 

  client(putBuffer);
}

void get(char key[], char value[])
{
	node x, y;
	x.ip[0] = '\0';
	x.port = 0;
	for(int i = 0; i < 20; ++i) value[i] = '\0';
  	
  	x.id = compute_md5((const char *)key, strlen(key));
  	/*for(int i = 0; i < 17; i++)
	{
		if( x.id[i] == '\0' || x.id[i] == ' ') x.id[i] = (char)255; 
	}*/
	// decimal_of_digest();
	y = findSuccessor(x);
	
	//printf("GET: key => [%s] hash of key => [%d] successor ip => [%s] and port => [%d]\n",key, x.id, y.ip, y.port);fflush(stdout);
	strcpy(targetIP, y.ip);
	targetPort = y.port;
  
	for(int i=0; i<255; ++i) getBuffer[i]='\0';

	sprintf(getBuffer,"getdata %d",x.id);
	
	client(getBuffer);  // Retrieve value corresponding to the key

	sscanf(getBuffer,"%s",value);
	if(value[0] == '\0') fprintf(stderr,"Value NOT found\n");
	else printf("Value for the key [%s] is [%s]\n",key,value);
	
}

void createRing()
{
	char id[100];
	//printf("In createRing()\n");fflush(stdout);
	
	for(int i=0; i<100; ++i)
	{
		id[i] = '\0';
	}
	
	sprintf(id,"%s:%d",mySelf.ip, mySelf.port);
	
	mySelf.id = compute_md5((const char*)id, strlen(id));
	/*for(int i = 0; i < 17; i++)
	{
		if( mySelf.id[i] == '\0' || mySelf.id[i] == ' ') mySelf.id[i] = (char)255; 
	}*/
	//decimal_of_digest();
	printf("My ID => [%d], ip => [%s], port => [%d] \n", mySelf.id, mySelf.ip, mySelf.port);
	successor.id = mySelf.id;
	predecessor.id = mySelf.id;
	fingerTable[1].id = mySelf.id;
	strncpy(successor.ip,mySelf.ip,16);
	strncpy(predecessor.ip,mySelf.ip,16);
	strncpy(fingerTable[1].ip,mySelf.ip,16);
	successor.port = mySelf.port;
	predecessor.port = mySelf.port;
	fingerTable[1].port = mySelf.port;
	temp.port = predecessor.port;
	temp.id = predecessor.id;
	strncpy(temp.ip,predecessor.ip,16);
	
	/*printf("CREATE: Predecessor successor information : \n");
	printf("Predecessor : ip %s, port %d\n",predecessor.ip, predecessor.port);
	printf("Successor : ip %s, port %d\n",successor.ip, successor.port);*/
	
	
	//create a server thread
	
	int stid = create(server);
	run(stid);
	int staid = create(stabilize);
	run(staid);
	int ffid = create(fixFinger);
	run(ffid);
	
	//puts(mySelf.ip);
	printf("CREATE: A new ring is created with ip:port = %s\n",id);fflush(stdout);
}
void server()
{
	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[256], response[256];
	struct sockaddr_in serverAddr, clientAddr;
	/*printf("Server : INITIAL successor predecessor value \n");
	printf("Successor: ID => %d IP => %s, port => %d\n", successor.id, successor.ip, successor.port);
	printf("Predecessor: ID => %d IP => %s, port => %d\n", predecessor.id, predecessor.ip, predecessor.port);fflush(stdout);*/
	
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if(sockfd < 0)
	{
		fprintf(stderr, "SERVER: ERROR, opening socket\n");
	}
	
	bzero((struct sockaddt_in*) &serverAddr, sizeof(serverAddr));
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(mySelf.port);

	if(bind(sockfd, (const struct sockaddr *) &serverAddr, (socklen_t)sizeof(serverAddr)) < 0)
	{ 
		fprintf(stdout,"SERVER: Socket already taken. Force taking...");fflush(stdout);      
		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
      		{
      			perror("setsockopt");
      			exit(1);
		}
	}

	while(1)
	{
		listen(sockfd,1000);
		
		clilen = sizeof(clientAddr);

		newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clilen);
		
		if(newsockfd < 0)
		{
			fprintf(stderr,"SERVER: ERROR, Not accepting new socket\n");
			exit(1);
		}
		
		//bzero(buffer,256);
		n = read(newsockfd,buffer,256);
		//printf("SERVER:CLient send ");puts(buffer);printf("\n");fflush(stdout);
		if (n < 0)
		{
			fprintf(stdout,"SERVER: ERROR, reading from socket\n");
			exit(1);
		}
		//printf("SERVER: Buffer is : ");puts(buffer);printf("\n");fflush(stdout);

		if(strncmp(buffer,"notify",6) == 0)
		{
			int i;
			char command[256];
			node x;
			char arr[20];
			i = 7;
			while(buffer[i] != ' ')
			{
				arr[i-7] = buffer[i];i++;
			}
			arr[i] = '\0';
	      		x.id = atoi(arr);
			//for(i = 7; i < 23 ; ++i) x.id = buffer[i];
			int j = 0;
			for(; i<255; ++i) command[j++] = buffer[i];
			sscanf(command,"%s %d",(x.ip),&(x.port));
			//printf("In server: %s %d\n",x.ip,x.port);
			notify(x);

			n = write(newsockfd,"I got your message",18);
			if (n < 0)
			{
				fprintf(stderr,"SERVER: ERROR, writing to socket\n");
				exit(1);
			}
		}
		
		/********************      TEST    *************************/
		else if(strncmp(buffer,"pred",4) == 0)
		{
			int i;
			char command[256];
			node x;
			char arr[20];
			//printf("SERVER: Setting my predecessor\n");fflush(stdout);
			i = 5;
			while(buffer[i] != ' ')
			{
				arr[i-5] = buffer[i];i++;
			}
			
			arr[i] = '\0';
	      	x.id = atoi(arr);
			
			int j = 0;
			for(; i<255; ++i) command[j++] = buffer[i];
			sscanf(command,"%s %d",(x.ip),&(x.port));
			//printf("SERVER: value of pred.id =>[%d], pred.ip=>[%s], pred.port=>[%d]", temp.id, temp.ip, temp.port);fflush(stdout);
			
			if(x.id != predecessor.id)
			{
				/****Telling previous predecessor to change it's successor***/
			
				
				if(predecessor.id != mySelf.id)
				{
					strncpy(targetIP, predecessor.ip,16);
					targetPort = predecessor.port;
					char ysucc[256];
					for(int i = 0; i < 255; i++) ysucc[i] = '\0';
					sprintf(ysucc,"succ %d %s %d", x.id, x.ip, x.port);
					client(ysucc);
				}
				
				/************************************************************/
			
				predecessor.id = x.id;
				strncpy(predecessor.ip,x.ip,16);
				predecessor.port = x.port;
				//printf("SERVER: setting predecessor ip to %s\n",x.ip);fflush(stdout);
			}
			n = write(newsockfd,"ok pred", 7);
			if (n < 0)
			{
				fprintf(stderr,"SERVER: ERROR, writing to socket\n");
				exit(1);
			}
		}
		
		else if(strncmp(buffer,"succ",4) == 0)
		{
			int i;
			char command[256];
			node x;
			char ypred[256];
			char arr[20];
			//printf("SERVER: Setting successor\n");fflush(stdout);
			i = 5;
			while(buffer[i] != ' ')
			{
				arr[i-5] = buffer[i];i++;
			}
			
			arr[i] = '\0';
	      	x.id = atoi(arr);
			
			int j = 0;
			for(; i<255; ++i) command[j++] = buffer[i];
			sscanf(command,"%s %d",(x.ip),&(x.port));
			
			for(int i = 0; i < 256; i++)
			{
				ypred[i] = '\0';
			}
			
			if(x.id != successor.id)
			{
				
				if(successor.id != mySelf.id)
				{
					targetPort = successor.port;
					strncpy(targetIP, successor.ip,16);
					sprintf(ypred, "pred %d %s %d",x.id, x.ip, x.port);
				
				
					//Telling previous successor to change it's predecessor
					client(ypred);
				}
				
				
				successor.id = x.id;
				successor.port = x.port;
				strncpy(successor.ip,x.ip,16);
				fingerTable[1].id = successor.id;
				fingerTable[1].port = successor.port;
				strncpy(fingerTable[1].ip, successor.ip, 16);
				
			}
			n = write(newsockfd,"ok succ", 7);
			if (n < 0)
			{
				fprintf(stderr,"SERVER: ERROR, writing to socket\n");
				exit(1);
			}
			
		}
		/**************************************************/
		
		else if(strncmp(buffer,"findPredecessor",15) == 0)
		{
			//printf("SERVER: predecessor IP %s and port %d\n", predecessor.ip,predecessor.port);fflush(stdout);
			sprintf(response,"%d %s %d",predecessor.id, predecessor.ip, predecessor.port);
			//for(int i = 0; i< 
			//printf("SERVER: RESPONSE "); puts(response); printf("\n"); fflush(stdout);
			n = write(newsockfd, response, 256);
		
			if (n < 0)
			{
				fprintf(stderr,"SERVER: ERROR, writing to socket\n");
				exit(1);
			}
		}
		else if(strncmp(buffer,"findSuccessor",13) == 0)
		{
			int i;
			char command[256];
			node x, y;
			char arr[20];
			i = 14;
			while(buffer[i] != ' ')
			{
				arr[i-14] = buffer[i];i++;
			}
			arr[i] = '\0';
	      		x.id = atoi(arr);
			
			
			//for(i=14; i < 30; ++i) x.id[i-14] = buffer[i];
			int j = 0;
			for(; i<255; ++i) command[j++] = buffer[i];
			sscanf(command,"%s %d",(x.ip),&(x.port));
			//printf("SERVER: x.id => %d x.ip => %s x.port => %d\n",x.id, x.ip, x.port);
			y = findSuccessor(x);

			sprintf(response,"%d %s %d",y.id,y.ip,y.port);
			/*printf("SERVER: server writes\n");
			puts(response);
			printf("\n");*/
			n = write(newsockfd, response, 256);
			if (n < 0)
			{
				fprintf(stderr,"ERROR writing to socket\n");
				exit(1);
			}
		}
	
		else if(strncmp(buffer,"getdata",7) == 0)
		{
			int i;
			char key[20], d[20];
			for(i=0; i<21; ++i)  key[i] = '\0';
			sscanf(buffer,"%s %s",d,key);
			getData(key);
			n = write(newsockfd, VAL, 20);
			if (n < 0)
			{
				fprintf(stdout,"ERROR writing to socket\n");
				exit(1);
			}
		}
	
		else if(strncmp(buffer,"putdata",7) == 0)
		{
			int i;
			char key[20], value[20], d[20];
			for(i=0; i<20; ++i)
			{
				key[i] = '\0';
				value[i] = '\0';
			}
			sscanf(buffer,"%s %s %s",d,key,value);
			/*printf("calling putData\nWith key = ");puts(key);
			printf("\nValue = ");puts(value);
			printf("\n");*/
	      
			putData(key,value);
		}
		else
		{
			printf("SERVER: I am not suppose to match this case\n");
			puts(buffer);printf("\n");
			fflush(stdout);
		}
		close(newsockfd);
	}
	close(sockfd);
}

void client(char buffer[256])
{
	int sockfd, n;
	struct sockaddr_in serverAddr;
	
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if(sockfd < 0)
	{
		fprintf(stderr,"Error opening socket\n");
		exit(1);
	}
	
	bzero((char *) &serverAddr, sizeof(serverAddr));
	
	/*printf("CLIENT: Target IP: \n");
	puts(targetIP);
	printf("CLIENT: Target Port = %d\n",targetPort);
	printf("\n");*/
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(targetPort);
	serverAddr.sin_addr.s_addr = inet_addr(targetIP);
	if(connect(sockfd,(struct sockaddr *) &serverAddr,sizeof(serverAddr)) < 0)
	{
		fprintf(stderr,"CLIENT: ERROR, Host with ip => [%s] and port => [%d] Network Error.\n",targetIP, targetPort);
		exit(1);
    }
    	
    /*printf("CLIENT: client writes \n");
    puts(buffer);
    printf("\n");*/
    n = write(sockfd,buffer, 256);
    	
	
	if (n < 0)
	{
		fprintf(stderr,"CLIENT: ERROR writing to socket\n");
	}
	
	bzero(buffer,256);
	//printf("CLIENT: before reading\n");fflush(stdout);
	n = read(sockfd,buffer,255);
	//printf("CLIENT: After reading\n");fflush(stdout);
	if (n < 0) 
		fprintf(stderr, "CLIENT: ERROR reading from socket");
	close(sockfd);
	return;
    
}

void stabilize()
{
	char command[256];
	node x;
	while(1)
	{
		//sleep(100);
		if(mySelf.id != successor.id || mySelf.id != fingerTable[1].id)
		{
			strcpy(targetIP, successor.ip);
			targetPort = successor.port;
			//printf("STABILIZE: Target IP %s and port %d\nsuccessor ip %s and successor port %d \n",targetIP,targetPort, successor.ip, successor.port);fflush(stdout);
			for(int i=0; i<255; ++i) predBuffer[i]='\0';
			strcpy(predBuffer,"findPredecessor");
			//printf("STABILIZE: predBuffer value : ");puts(predBuffer);printf("\n");fflush(stdout);
			client(predBuffer);
			
			char arr[20];
			int i = 0;
			while(predBuffer[i] != ' ')
			{
				arr[i] = predBuffer[i];i++;
			}
			arr[i] = '\0';
	      		x.id = atoi(arr);
			
			//for(int i = 0; i < 17; i++) x.id[i] = predBuffer[i];
			int j = 0;
			for(; i < 256;i++) command[j++] = predBuffer[i];
			sscanf(command,"%s %d",x.ip, &(x.port));
			for(int i=0; i<255; ++i) predBuffer[i]='\0';
		
			if(x.port != 0 && inBetween(x.id,mySelf.id,successor.id,0))
			{
				successor.id = x.id;
				fingerTable[1].id = x.id; 
				strcpy(successor.ip, x.ip);
				strcpy(fingerTable[1].ip,x.ip);
				successor.port = x.port;
				fingerTable[1].port = x.port; 
				//printf("STABILIZE: successor updated to %s:%d\n",successor.ip,successor.port);
			}
			
			//call for notify
			strcpy(targetIP,successor.ip);
			targetPort = successor.port;
			// printf(" Contact for notify target IP %s and target Port %d\nsuccessor ip %s and successor port %d \n",targetIP, targetPort, successor.ip, successor.port);
			for(int i=0; i<255; ++i) notBuffer[i]='\0';
			sprintf(notBuffer,"notify %d %s %d", mySelf.id, mySelf.ip, mySelf.port);
			//Tell successor to check if I am your predecessor.
			client(notBuffer);
			
			for (int i = 0; i < 256; i++) notBuffer[i] = '\0';
		}
		sleep(10);
	}
}

void notify(node x)
{
	if(mySelf.id == x.id) return;
	
	if((predecessor.port == 0) ||(inBetween(x.id, predecessor.id, mySelf.id, 0)))
	{
		predecessor.id = x.id;
		strncpy(predecessor.ip, x.ip, 16);
		predecessor.port = x.port;
		printf("NOTIFY: Setting Predecessor to %s:%d\n", predecessor.ip, predecessor.port);
		fflush(stdout);
	}
}

void join(node x)
{
	char command[256];
	char id[100];
	for(int i=0; i<100; ++i)
	{
		id[i] = '\0';
	}
	
	sprintf(id,"%s:%d",mySelf.ip,mySelf.port);
	
	mySelf.id = compute_md5((const char *)id,strlen(id));
	strncpy(targetIP, x.ip, 16);
	targetPort = x.port;
	printf("My Id is %d\n",mySelf.id);
	successor.id = mySelf.id;
	predecessor.id = mySelf.id;
	fingerTable[1].id = mySelf.id;
	strncpy(successor.ip,mySelf.ip,16);
	strncpy(predecessor.ip,mySelf.ip,16);
	strncpy(fingerTable[1].ip,mySelf.ip,16);
	successor.port = mySelf.port;
	predecessor.port = mySelf.port;
	fingerTable[1].port = mySelf.port;
	
	/*printf("JOIN: Initial Predecessor successoor information : \n");
	printf("Predecessor : ip %s, port %d\n",predecessor.ip, predecessor.port);
	printf("Successor : ip %s, port %d\n",successor.ip, successor.port);fflush(stdout);*/
	
	for(int i=0; i<255; ++i) succBuffer[i]='\0';
	
	sprintf(succBuffer, "findSuccessor %d %s %d", mySelf.id, mySelf.ip, mySelf.port);	//Tell me my successor
	/*printf("JOIN: succBuffer \n");
	puts(succBuffer);
	printf("\n");*/
	client(succBuffer);
	/*printf("JOIN: Client returned to findSuccessor \n");
	puts(succBuffer);printf("\n");fflush(stdout);*/
	
	int j;char arr[20];
	for (int i =0; succBuffer[i] != ' '; i++)
	{
		arr[i] = succBuffer[i];
		succBuffer[i] = '\0';
		j = i;
	}
	
	successor.id = atoi(arr);
	
	/*for(int i=0; i<17; ++i)
	{
		successor.id[i] = succBuffer[i];
		succBuffer[i] = '\0';
	}*/
	int k = 0;
	for(int i = j+2; i<255; ++i)
	{
		command[k++] = succBuffer[i];
		succBuffer[i] = '\0';
	}
	//printf("Command is : ");puts(command);printf("\n");fflush(stdout);
	sscanf(command,"%s %d",(successor.ip),&(successor.port));
	fingerTable[1].id = successor.id;
  	strcpy(fingerTable[1].ip, successor.ip);
  	fingerTable[1].port = successor.port;
 
/*******************       TEST    ***********************/  	
  	char ypred[256];
  	for(int i=0;i<256;i++) ypred[i] = '\0';
  	sprintf(ypred,"pred %d %s %d", mySelf.id,mySelf.ip, mySelf.port);
	strncpy(targetIP, successor.ip, 16);
	targetPort = successor.port;
	client(ypred);
  	
 /**************************************************/ 	
  	
  	printf("JOIN: Ring joined via node IP : %s at Port:%d.\n",successor.ip, (successor.port));fflush(stdout);
  	
	  int sid = create(server);// thread for server; call its run
	run(sid);
  
	int staid = create(stabilize);// create thread for satbilize
	run(staid);

	int fixfingerid = create(fixFinger);// create thread for fixFinger
	run(fixfingerid);
}

node findSuccessor(node x)
{
	static int i = 0;
	if(/*mySelf.id == successor.id ||*/ inBetween(x.id, mySelf.id, successor.id, 1))
	{
		//printf("FINDSUCCESSOR: If condition met return successor ip => [%s], port => [%d]\n",successor.ip, successor.port);fflush(stdout);
		//printf("FINDSUCCESSOR:%d returns id=>[%d], ip=>[%s], port=>[%d]\n", i++, successor.id, successor.ip, successor.port);
		return successor;
	}
	
	else
	{
		int i;
		node t, u;
		
		//printf("FINDSUCCESSOR: before preceding node\n");
		t = closestPrecedingNode(x);
		
		//printf("findsuccessor: CPN returns id=>[%d], ip=>[%s], port=>[%d]\n",t.id, t.ip, t.port);fflush(stdout);
		if((strcmp(t.ip, mySelf.ip) == 0) && (t.port == mySelf.port)) return successor;
		
		strcpy(targetIP, t.ip);
		targetPort = t.port;
		for(i=0; i<256; ++i) succBuffer[i]='\0';
		sprintf(succBuffer,"findSuccessor %d %s %d",t.id,t.ip,t.port);
		
		
		//printf("FINDSUCCESSOR: Before segfault convict\n");puts(succBuffer);fflush(stdout);
		client(succBuffer);
		//printf("FINDSUCCESSOR: After segfault convict\n");fflush(stdout);
		sscanf(succBuffer, "%d %s %d", &(u.id), u.ip, &(u.port));
		//printf("%d %s %d\n", u.id, u.ip, u.port);
		
		for(i = 0; i < 256; i++) succBuffer[i] ='\0';
		//printf("FINDSUCCESSOR: else condition met return successor ip => [%s], port => [%d]\n",u.ip, u.port);fflush(stdout);
		return u; 
	}
}

node closestPrecedingNode(node x)
{
	int i;  //fprintf(stdout,"closest\n");
  	for(i=32; i>0; --i)
  	{
  		//printf("CPN: %d",i);fflush(stdout);
    	if(inBetween(fingerTable[i].id, mySelf.id, x.id, 0) && fingerTable[i].port != 0)
    	{
      		return (fingerTable[i]);
		}
	}
	return (fingerTable[1]);
}

void fixFinger()
{
	node x;
	int next = 0;
	//printf("here\n");fflush(stdout);
	while(1)
	{
		next = next + 1;
		if(next > 32)
		{
			next = 1;
			//int i = 0;
			sleep(10);
		}
		
		x.id = (long)(mySelf.id + pow(2,next-1)) % 65535;
		//printf("x.id = %d\n",x.id);
		fingerTable[next] = findSuccessor(x);
		
	}
}

void getInput()
{
	char command[101], temp[101];
	printf("NOTE: This is CSP-701 Software System Laboratory\n");
	printf("Assignment-2 Distributed Hash Table(Chord)\n");
	printf("Type 'help' for commands\n\n");
	while(1)
	{
		printf("Command > ");
		fgets(command, 100, stdin);
		if(strncmp(command,"help",4) == 0)
		{      
			printf("Commands:\n1)create(to create a ring)\n2)get(to get data corresponding)\n3)join(To join into ring)\n4)port(to set port)\n5)put(to put data into ring)\n6)fingers\n7)quit(to quit the ring)\n");
		}

		else if(strncmp(command,"quit",4) == 0)
		{
			printf("Quitting the ring.\n");exit(0);
		}
		else if(strncmp(command,"port",4) == 0)
		{
			sscanf(command,"%s %d",temp, &(mySelf.port));
			if(mySelf.port < 1024 || mySelf.port > 65535)
			{
				printf("Invalid port number. Setting default port 4444\n");
				mySelf.port = 4444;
			}

			else
			{
				printf("Port value is set to %d\n",mySelf.port);

			}

		}

		else if(strncmp(command,"create",6) == 0)
		{
			createRing();
		}
		
		else if(strncmp(command,"join",4) == 0)
		{
			node x;
			sscanf(command,"%s %s %d", temp, x.ip, &(x.port));
			join(x);
		}
		
		else if(strncmp(command,"put", 3) == 0)
		{
			char key[20];
			char value[20];
			int i;
			for(i = 0; i < 20; i++)
			{
				key[i] = '\0';
				value[i] = '\0';
			}
			
			sscanf(command, "%s %s %s",temp, key,value);
			put(key, value);
		}
		
		else if(strncmp(command,"get",3) == 0)
		{
			char key[20];
			char value [20];
			int i;
			for(i = 0; i < 20; i++)
			{
				key[i] = '\0';
				value[i] = '\0';
			}
			
			sscanf(command,"%s %s",temp, key);
			get(key, value); 
		}
		
		else if(strncmp (command,"finger",6) == 0)
		{
			for(int i = 1; i < 33; i++)
			{
				printf("fingerTable[%d] => %d\n",i, fingerTable[i].id);
			}
		}
		
		else if(strncmp(command, "dump",4) == 0)
		{
			printf("-------NODE INFO---------\n");
			printf("Successor at - %s : %d \n",successor.ip,successor.port);
			printf("Predecessor at - %s : %d \n",predecessor.ip,predecessor.port);
			
			printf("---------Finger table entries------\n");
			for(int i = 1 ; i < 33 ; i++)
			{
				printf("---------fingerTable[%d]  -------\n",i);
				printf("IP = %s . PORT = %d . ID = %d \n",fingerTable[i].ip,fingerTable[i].port,fingerTable[i].id);				
			}
			
		}
		
		else if(strncmp(command, "predecessor",11) == 0)
		{
			printf("Predecessor is ip => [%s] and port => [%d] \n",predecessor.ip,predecessor.port);
			
		}
		
		else if(strncmp(command, "successor",9) == 0)
		{
			printf("Successor is ip => [%s] and port => [%d] \n",successor.ip,successor.port);
			
		}
		
		else
		{
			printf("Command not valid try again\n");
		}

	}
}

int main()
{
	int i;
	char* ip = NULL;
	
	/*printf("ip = %s length = %lu port = %d\n", mySelf.ip, strlen(mySelf.ip), mySelf.port);
	fflush(stdout);*/
	
	//set predecessor to null
	predecessor.id = 0;
	for(int i=0;i<17;i++)predecessor.ip[i] = '\0';
	predecessor.port = 0;
	//printf("here 1\n");fflush(stdout);
	//set finger table to null
	for(i=1 ;i < 33;++i)
	{
		fingerTable[i].id = 0;
		fingerTable[i].ip[0] = '\0';
		fingerTable[i].port = 0;
	}
	//Initialize Hash Table
	for(i =1;i < 100; ++i)
	{
		table[i].key[0] = '\0';
		table[i].value[0] = '\0';
		table[i].available = 1;
	}
	ip = getIPAddr();
	sprintf(mySelf.ip,"%s",ip);
	mySelf.port = 1111;
	printf("ip = %s port = %d\n", mySelf.ip, mySelf.port);
	//puts(mySelf.ip);
	create(getInput);
	start();
	//puts(mySelf.ip);
}

DEP = ./MyThread.h
CFL = -fPIC -c -O2 -I ../
COMP = g++

all:	dht

dht:	$(DEP) dht.c MyThread.c
	$(COMP) dht.c MyThread.c -lcrypto -lm -o dht
	
clean:
	rm -rf dht
	
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "MyThread.h"

#define SECOND 1
#define STACK_SIZE 4096

static int currentThread = -1;
static int nThreads = 0;
static TCB threadList[MAXTHREADS];
static int ran = 0;

//static int count_dead = MAXTHREADS;

static void wrapper();
void alarm_handler(int sig);
void initStatistics(struct statistics* stat,int id);
void clean();
void dispatch(int sig);
void yield();
void deleteThread(int threadID);
int createWithArgs(void *(*f)(void *), void *args);
int create(void (*f)(void));
int getID();
void run(int tid);
void suspend(int tid);
void resume(int tid);
void sleep(int secs);
void start(void);

//char stack1[STACK_SIZE];
//char stack2[STACK_SIZE];
//sigjmp_buf env[2];

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

void initStatistics(struct statistics* stat,int id)
{
	assert(stat != NULL);
	stat->ID = id;
	stat->state = DED;
	stat->burst = 0;
	stat->total_exec_time = 0;
	stat->total_slp_time = 0;
	stat->avg_time_quant = 0;
	stat->avg_wait_time =  0;
	stat->RedTimeTotal = 0;
	//stat->slptime = 0;

}

void clean()
{
    int count = 0;
    for(int i = (currentThread+1)%MAXTHREADS; count < MAXTHREADS; i = (i+1)%MAXTHREADS)
    {
        if(threadList[i].stat.state != DED)
        {
            threadList[i].stat.avg_wait_time = threadList[i].stat.RedTimeTotal / threadList[i].stat.burst;
            threadList[i].stat.avg_time_quant = threadList[i].stat.total_exec_time / threadList[i].stat.burst;
            printf("Thread ID: %d\nTotal execution time: %d\nTotal Sleep Time: %d\nTotal bursts: %d\nAverage waiting time: %f\nAverage Time Quanta: %f\n\n\n\n\n",i,threadList[i].stat.total_exec_time, threadList[i].stat.total_slp_time, threadList[i].stat.burst,threadList[i].stat.avg_wait_time,threadList[i].stat.avg_time_quant);
            threadList[i].stat.state = DED;
            count++;
        }
    }
    exit(0);
}

void dispatch(int sig)
{
  int count = 0;
  signal(SIGALRM,SIG_IGN);
  
 if(threadList[currentThread].stat.state != DED)
 { 
    int ret_val = sigsetjmp(threadList[currentThread].env,1);
    if (ret_val == 1) {
      return;
    }
  
    threadList[currentThread].stat.state = RED;
    threadList[currentThread].stat.total_exec_time += (clock() - threadList[currentThread].stat.RunTimeStart)/CLOCKS_PER_SEC;
    threadList[currentThread].stat.RedTimeStart = clock();
  }
  for(int i = (currentThread+1)%MAXTHREADS; count <= MAXTHREADS; i = (i+1)%MAXTHREADS)
        {
	        if(threadList[i].stat.state == RED)
            {
	            currentThread = i;
                break;
	        }
	        count++;
	     }
	     if(count > MAXTHREADS) exit(0);
	     
	     threadList[currentThread].stat.state = RUN;
	     threadList[currentThread].stat.RedTimeTotal += (clock() - threadList[currentThread].stat.RedTimeStart)/CLOCKS_PER_SEC;
	     threadList[currentThread].stat.RunTimeStart = clock();
         threadList[currentThread].stat.burst++;   
         signal(SIGALRM, alarm_handler);
         alarm(1);
         siglongjmp(threadList[currentThread].env,1);
}

void yield(){
  dispatch(SIGALRM);
}

void deleteThread(int threadID)
{
    nThreads --;
    
    threadList[threadID].stat.state = DED;
    printf("Thread with %d id is deleted\n",threadID);
}

static void wrapper()
{
    if(threadList[currentThread].tType == ARG)
    {
        threadList[currentThread].retVal = threadList[currentThread].f2(threadList[currentThread].args);
    }
    else
    {
        threadList[currentThread].f1();
    }
  printf("Thread %d exited\n",getID());
  deleteThread(currentThread);
  
  //currThread = NULL;//threadMAP[1];
 // if(count_dead < MAXTHREADS)
    signal(SIGALRM,alarm_handler);
    alarm(1);
    dispatch(SIGALRM);
}

int createWithArgs(void *(*f)(void *), void *args)
{
    int id=-1;
    if(ran==0)
    {
        for(int i = 0; i < MAXTHREADS; i++)
            initStatistics(&(threadList[i].stat),i);
        ran = 1;
    }
	for(int i = 0; i < MAXTHREADS; i++)
 	{	
 	    if(threadList[i].stat.state==DED)
			{
			    id = i;
			    break;
			}
	}
    if(id == -1)
	{
		fprintf(stderr, "Error Cannot allocate id\n");
		return id;
		
	}
    assert(id >= 0 && id < MAXTHREADS);
    
    //address_t sp, pc;
   //sp = (address_t)stack + 4096 - sizeof(address_t);
    //pc = (address_t)wrapper;
    //sigsetjmp(threadList[currentThread].env,1);
    threadList[id].f2 = f;
    threadList[id].args = args;
   // sigemptyset(&tcb->context->__saved_mask);
  
    
	threadList[id].stat.state = RED;  
	nThreads++;
	//count_dead--;
	return  id;
}


int create(void (*f)(void))
{
//TODO - update data structures 
	int id=-1;
	if(ran == 0)
	{
	    for(int i = 0; i < MAXTHREADS; i++)
            initStatistics(&(threadList[i].stat),i);
        ran = 1;
	}
	for(int i = 0; i < MAXTHREADS; i++)
 	{	
 	    if(threadList[i].stat.state==DED)
			{
			    id = i;
			    break;
			}
	}
	
	if(id==-1)
	{
		fprintf(stderr, "Error Cannot allocate id");
		return id;
		
	}
	assert(id >= 0 && id < MAXTHREADS);
	threadList[id].tType = NOARG;
	threadList[id].f1 = f;
	threadList[id].stat.state = NEW;  
	nThreads++;
	//count_dead--;
	return  id;
}

void alarm_handler(int sig)
{
	signal(SIGALRM, alarm_handler);
	dispatch(SIGALRM);
}



int getID()
{
	return currentThread;
}

void run(int tid)
{
	threadList[tid].stat.state = RED;
}

void suspend(int tid)
{
	threadList[tid].stat.state = SUS;
}

void resume(int tid)
{
	threadList[tid].stat.state = RED;
}

void sleep(int secs)
{
	threadList[currentThread].stat.state = SLP;
	//threadList[currentThread].stat.slptime = secs;
	//threadList[currentThread].stat.start_time_slp = clock();
	clock_t StartTime = clock();
    while(((clock()-StartTime)/CLOCKS_PER_SEC) <= secs);
    threadList[currentThread].stat.total_slp_time += secs;
	
	dispatch(SIGALRM);
	 
	
}


struct statistics* getStatus(int tid)
{
	struct statistics* result = &(threadList[tid].stat);
	return result;
}


void start(void)
{
    address_t sp, pc;
    for(int i = 0; i < MAXTHREADS; i++)
    {
        sp = (address_t)threadList[i].stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t)wrapper;
        sigsetjmp(threadList[i].env, 1);
        (threadList[i].env->__jmpbuf)[JB_SP] = translate_address(sp);
        (threadList[i].env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&(threadList[i].env)->__saved_mask);
        if(threadList[i].stat.state == NEW)   threadList[i].stat.state = RED; 
    }
    signal(SIGALRM,alarm_handler);
    alarm(1);
    while(1);
}

#ifndef MYTHREAD_H

#define MYTHREAD_H
#include"allstdlib.h"
#include<setjmp.h>
#define MAXTHREADS 100

typedef void *(*f_WithArgs)(void *);
enum STATES{DED=0,RUN,RED,SLP,SUS, NEW};
enum BOOLEAN {FALSE=0,TRUE};
enum TYPE {ARG = 0, NOARG};

struct statistics
{
	int ID;
	enum STATES state;
	int burst;
	int total_exec_time;
	int total_slp_time;
	float avg_time_quant;
 	float avg_wait_time;
 	int RedTimeTotal;
 	clock_t RunTimeStart;
 	clock_t RedTimeStart;
 	//int slptime;
 	//clock_t start_time_slp;
};

typedef struct tcb
{
	jmp_buf env;
	char stack[4096]; //the thread stack
	struct statistics stat;
	enum TYPE tType;//thread type determining arg/noarg
	void (*f1)(void);//thread routine no arg type
	f_WithArgs f2;//thread routine arg type
	void* args;//thread args
	void* retVal;
	
}TCB; //the thread control block

int create(void (*f)(void));//update dses of the control
int createWithArgs(void *(*f)(void *), void *arg);
int getID(void);//return RThreadID
void dispatch(int);//the scheduler func, sig == signalID?
void start(void);//start executing the threads - possibly a master thread???
void run(int threadID);//put thread status = RED?
void suspend(int threadID);//put thread status = SUS?
void resume(int threadID);//put thread status = RED?
void yield(void);
void initStatistics(struct statistics* stat,int id);
void deleteThread(int threadID);
void sleep(int sec);// put thread status = SLP?
struct statistics* getStatus(int threadID);//print the fields of statistics??
int createWithArgs(void*(*f)(void*),void* arg);
void clean(void);//stops the master thread?
void JOIN(int threadID);//bonus
void* GetThreadResult(int threadID);//bonus
#endif	
