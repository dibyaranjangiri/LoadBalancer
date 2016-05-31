/*
* This is Loadbalancer server which handles the clients 
* requests by getting data from dataserver
* file: loadserver.c
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
//#include <loadserver.h>
//#include <log.h>

void* writerThread(void* parm);          /* writerThread function prototype */
void* readerThread(void* parm);          /* readerThread function prototype */
void* clientHandlerThread(void* parm);   /* load balancer main thread function prototype */
void* serverHandlerThread(void* parm);   /* data server handler thread function prototype */
void* clientHandlerThread(void* parm);   /* client handler thread function prototype */
void* serverHandlerThread(void* parm);   /* data server handler thread function prototype */
void circularBuf_init();

#define CLIENTPORT	9801    /* client port number */
#define SERVERPORT  9802    /* data server port number */
#define CLISTENQ    100     /* size of client listner queue   */
#define SLISTENQ    5       /* size of data server listner queue */
#define BufferSize  35      /* can store 35 client requests */

#define BNUM 	100
#define BSIZE 	7
#define actualPopSize 7
#define pushSize 7 

static pthread_t  client_tid ,server_tid;  /* variable to hold thread ID */
pthread_attr_t attrs;


typedef struct {
	pthread_mutex_t lock;   
	pthread_cond_t
    new_data_cond,
    new_space_cond;
    char *circularBuf;
    int producerPointer;
    int  consumerPointer;
    int  count;
}circularBuf_t;

circularBuf_t *cbuf;           /* circular buffer */

int main(int argc, char *argv[])
{
	int retVal;
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	circularBuf_init();  
	/* start with default attributes */
    pthread_attr_init (&attrs);
    /* add an attribute that will cause us to use all processors
      if we have a machine with multiple processors
    */
    //pthread_attr_setscope (&attrs, PTHREAD_SCOPE_SYSTEM);
	/*started client MAIN thread*/
	if ( retVal = pthread_create(&client_tid, &attrs, clientHandlerThread, NULL) )
	{
		fprintf (stdout, "client pthread_create failed with error: %d\n", retVal);
		exit (1);
	}
	/*started data server MAIN thread*/
	if ( retVal = pthread_create(&server_tid, &attrs, serverHandlerThread, NULL) )
	{
		fprintf (stdout, "Data server pthread_create failed with error: %d\n", retVal);
		exit (1);
	}
	pthread_join(client_tid, NULL);
	pthread_join(server_tid, NULL);
	pthread_exit(NULL);

	free(cbuf->circularBuf); /*free the circularBuf*/
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
	return 0;

} /*end of main function*/


void circularBuf_init()                
{
	
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	cbuf=(circularBuf_t*)malloc(sizeof(circularBuf_t)); 
	cbuf->circularBuf=(unsigned char*)malloc(35);
    memset(cbuf->circularBuf, '\0', BufferSize); 
    cbuf->producerPointer = cbuf->consumerPointer = cbuf->count = 0;
    pthread_mutex_init(&cbuf->lock, NULL);
    pthread_cond_init(&cbuf->new_data_cond, NULL);
    pthread_cond_init(&cbuf->new_space_cond, NULL);
    fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
}


/*********************
* It is a concurrent server thread running to listing client request and 
* creating new thread to serve it.
* 
*********************/
void* clientHandlerThread(void* parm)
{
    struct   hostent   *ptrh;     /* pointer to a host table entry */
    struct   protoent  *ptrp;     /* pointer to a protocol table entry */
    struct   sockaddr_in saddr;   /* structure to hold server's address */
    struct   sockaddr_in caddr;   /* structure to hold client's address */
    int      sfd, cfd;            /* socket descriptors */
    int      port;                /* protocol port number */
    int      alen;                /* length of client address */
    int      ret;                 /* pthread_create return value*/ 
    int      reuse=1;
    pthread_t  tid;               /* variable to hold thread ID */
    pthread_attr_t attr;
    int* fd;
	socklen_t socksize = sizeof(struct sockaddr_in);
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
    memset((char  *)&saddr, 0, sizeof(saddr)); /* clear sockaddr structure   */
    saddr.sin_family = AF_INET;            /* set family to Internet     */
    saddr.sin_addr.s_addr = INADDR_ANY;    /* set the local IP address */
	port = CLIENTPORT;                     /* use CLIENTPORT = 9801  port number   */
	if (port > 0)                          /* test for illegal value    */
        saddr.sin_port = htons((u_short)port);
    else                                   /* print error message and exit */
	{                                
		fprintf (stdout, "bad port number %d\n",port);
        exit (1);
    }
	if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
	{
    	fprintf(stdout, "cannot map \"tcp\" to protocol number");
		exit (1);
	}
    // Create a TCP socket 
    sfd = socket (PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sfd == -1)
	{
		fprintf(stdout, "socket creation failed Error: %s\n",strerror(errno));
		exit(1);
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))< 0)
    {
        fprintf(stdout, "setsockopt failed Error:%s\n",strerror(errno));
        close(sfd);
        exit(2);
    }
	// Bind a local address to the socket 
    if (bind(sfd, (struct sockaddr *)&saddr, sizeof (saddr)) == -1)
	{
    	fprintf(stdout, "bind failed Error: %s\n", strerror(errno));
        exit(1);
	}
    // Specify a size of request queue 
	if (listen(sfd, CLISTENQ) < 0)
	{
    	fprintf(stdout,"listen failed\n");
        exit(1);
	}
	alen = sizeof(caddr);
    pthread_attr_init (&attr);
    fd=(int*)malloc(sizeof(*fd));
	while (1)
	{
    	printf("SERVER: Waiting for contact ...\n");
        if (  (cfd=accept(sfd, (struct sockaddr *)&caddr, &alen)) < 0)
		{
			fprintf(stdout, "accept failed\n");
            exit (1);
		}

		fprintf(stdout,"dibya..Connection from IP: %s \n", inet_ntoa(caddr.sin_addr));
   		*fd=cfd;
   		if ( ret = pthread_create(&tid, &attr, writerThread, fd ) )
		{
			fprintf (stdout, "clientHandlerThread pthread_create failed with error: %d\n", ret);
			//exit (-1);  // for one failure we can't died here
		} 
	} //end of while

	close(sfd); /*close server main FD*/
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);

} // end of clientHandlerThread function 


/*********************
* Receive the data from client and write the packet into circular buffer
* This acts like a producer
*********************/

void* writerThread(void* parm)
{
	int sockfd,k;
    char msg[100] = {'\0'};
    sockfd = *((int*) parm);
    int timeout=0,bytes;

	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	fprintf(stdout, "Writer received FD: %d\n",sockfd);
	while(1)
	{
    	bytes=recv(sockfd, msg, sizeof(msg), 0);
    	if (bytes == 0)
    	{
			sleep(1);
        	timeout++;
			continue;
    	}
    	if (timeout == 5)  //need to check select api
    	{
             
			fprintf(stdout,"dibya...Connection closing for FD=%d\n",sockfd);
        	close(sockfd);
    		pthread_exit(NULL); 
    	}
    	fprintf(stdout, "Writer received msg: %s,msglen: %d bytes=%d\n",msg,strlen(msg),bytes);
    	sprintf(msg+strlen(msg), ".%d", sockfd);  //append FD in msg

    	while (cbuf->count == BNUM)                              
    	pthread_cond_wait(&cbuf->new_space_cond, &cbuf->lock);

    	pthread_mutex_unlock(&cbuf->lock);                       
    	k = cbuf->producerPointer;
    	memcpy(cbuf->circularBuf+k, msg, strlen(msg));
    	cbuf->producerPointer = (cbuf->producerPointer + 1) % BNUM;
    	pthread_mutex_lock(&cbuf->lock);
    	cbuf->count+=strlen(msg);
    	fprintf(stdout, "producerPointer=%d count=%d \n",cbuf->producerPointer,cbuf->count);
    	pthread_mutex_unlock(&cbuf->lock);
    	pthread_cond_signal(&cbuf->new_data_cond);  
	} //end of while
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
} // end of writerThread 


/*********************
* This routine, Read the data block from circular buffer and send to dataserver.
* After getting data from dataserver, send the data to client
* This acts like consumer
*********************/

void* readerThread(void* parm)
{
	int sockfd;
	int stripfd;
	int bytes;
	char msg [100]={'\0'};
	char buf[100]={'\0'};
	sockfd = *((int*) parm);
    char *pch;
	int k;
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	fprintf(stdout, "Reader received FD: %d\n",sockfd);
	while(1)
	{
		//pthread_mutex_lock(&bufMutex);
		while (cbuf->count == 0)                                   
    		pthread_cond_wait(&cbuf->new_data_cond, &cbuf->lock);      
 
    	pthread_mutex_unlock(&cbuf->lock);                         
    	k = cbuf->consumerPointer;
		fprintf(stdout, "actualPopSize=%d consumerPointer=%d\n",actualPopSize,cbuf->consumerPointer);
    	memcpy (buf, (cbuf->circularBuf)+k, actualPopSize);
    	fprintf(stdout,"Reader Consumed data: %s\n",buf);
    	cbuf->consumerPointer = (cbuf->consumerPointer + 1) % BNUM;
    	pthread_mutex_lock(&cbuf->lock);                           
    	cbuf->count-=strlen(buf);                                             
    	pthread_cond_signal(&cbuf->new_space_cond);
   
    	pch = strtok(buf,".");
    	sprintf(msg,"%s",buf);
    	pch = strtok(NULL,".");
    	sprintf(msg+strlen(msg),".%s",pch);

    	pch = strtok(NULL,".");
    	stripfd = atoi(pch);
    	fprintf(stdout,"Reader stripped FD: %d\n",stripfd);
    	fprintf(stdout,"Reader before send msg: %s\n",msg);
 
    	bytes = send(sockfd, msg, strlen(msg)+1,0);  //send to data server      
    	fprintf(stdout,"Reader send msg to dataserver: %s\n",msg); 
        
    	memset(msg,'\0',sizeof(msg));
		bytes = recv(sockfd, msg, sizeof(msg), 0);  // recv from data server
    	//fprintf(stdout,"Reader recv msg from dataserver: %s\n",msg);
		bytes = send(stripfd, msg, strlen(msg)+1, 0);  // send data to client
		fprintf(stdout,"Reader send msg to client: %s\n",msg);
	} // end of while

	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
} // end of readerThread 

/*********************
 It is a concurrent server thread running is listing data server requests and 
 creating new thread to serve it.
 
*********************/
void* serverHandlerThread(void* parm)
{
    struct   hostent   *ptrh;     /* pointer to a host table entry */
    struct   protoent  *ptrp;     /* pointer to a protocol table entry */
    struct   sockaddr_in saddr;   /* structure to hold server's address */
    struct   sockaddr_in caddr;   /* structure to hold client's address */
    int      sfd, cfd;            /* socket descriptors */
    int      port;                /* protocol port number */
    int      alen;                /* length of client address */
    int      ret;                 /* pthread_create return value*/
    int      reuse=1;
    int* fd;
    pthread_attr_t attr;
    pthread_t  tid;               /* variable to hold thread ID */
   
    socklen_t socksize = sizeof(struct sockaddr_in);
 
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);

    memset((char  *)&saddr, 0, sizeof(saddr)); /* clear sockaddr structure   */
    saddr.sin_family = AF_INET;             /* set family to Internet     */
    saddr.sin_addr.s_addr = INADDR_ANY;     /* set the local IP address */
    port = SERVERPORT;                     /* use SERVERPORT = 9802 port number   */
    if (port > 0)                           /* test for illegal value    */
        saddr.sin_port = htons((u_short)port);
    else                                   /* print error message and exit */
    {                                
        fprintf (stdout, "bad port number %d/n",port);
        exit (1);
    }
    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
    {
        fprintf(stdout, "cannot map tcp to protocol number");
        exit (1);
    }
    /* Create a TCP socket */
    sfd = socket (PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sfd < 0)
    {
        fprintf(stdout, "socket create failed\n");
        exit(1);
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))< 0)
    {
        fprintf(stdout, "setsockopt failed Error:%s\n",strerror(errno));
        close(sfd);
        exit(2);
    }
    /* Bind a local address to the socket */
    if (bind(sfd, (struct sockaddr *)&saddr, sizeof (saddr)) == -1)
    {
		fprintf(stdout, "bind failed in server serverHandlerThread Error: %s\n",strerror(errno));
        exit(1);
    }
    /* Specify a size of request queue */
    if (listen(sfd, SLISTENQ) < 0)
    {
        fprintf(stdout,"listen failed\n");
        exit(1);
    }
    alen = sizeof(caddr);
    fd=(int*)malloc(sizeof(*fd));
    pthread_attr_init (&attr);
    while (1)
    {
    	printf("DATA SERVER: Waiting for contact ...\n");
    	if (  (cfd=accept(sfd, (struct sockaddr *)&caddr, &alen)) < 0)
		{
			fprintf(stdout, "DATA SERVER accept failed\n");
			exit (1);
		}

		fprintf(stdout,"Connection from IP: %s \n", inet_ntoa(caddr.sin_addr));

		*fd=cfd;
		fprintf(stdout, "FD=%d DATA SERVER accepted!\n",cfd);
		if ( ret = pthread_create(&tid, &attr, readerThread, fd) )
		{
			fprintf (stdout, " serverHandlerThread pthread_create failed with error: %d\n", ret);
			//exit (-1);   for one failure we can't exit
		}	 
	} //end of while//
	close(sfd); //close server handler main thread
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
}// end of serverHandlerThread function //
