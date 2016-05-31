/*
* file: dataserver.c
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define TABLESZ 10
#define INIT_EMPID 100
#define BUFSZ 100
#define SERVERERR_MSG "Data not found in server"
#define CONFIG_FILE_PATH "serverconfig.txt"	//contains server IP and PORT 


void readServerDetails();
void lookOnEmpTable(unsigned int eid, unsigned int rid, unsigned char* buf);

char serverip[16]={'\0'};
unsigned int port;

typedef struct
{
	unsigned int empid;
	char dname[50];
	char creden[50];
	char emailid[50];
}EmpTable;

enum RequestID
{
	dependent=1,
	credential,
	mailid
}REQ;

/*sample employee datasets*/
EmpTable emp[TABLESZ] =
{
	{100, "Mr Dhoni", "dhoni100", "dhoni100@team.com"},
	{101, "Mr Sachin", "sachin101", "sachin101@team.com"},
	{102, "Mr Ganguly", "ganguly102", "ganguly102@team.com"},
	{103, "Mr Dravid", "dravid103", "dravid103@team.com"},
	{104, "Mr Kumble", "kumble104", "kumble104@team.com"},
	{105, "Mr Jahir", "jahir105", "jahir105@team.com"},
	{106, "Mr Kohili", "kohili106", "kohili106@team.com"},
	{107, "Mr Raina", "raina107", "raina107@team.com"},
	{108, "Mr Jadeja", "jadeja108", "jadeja108@team.com"},
	{109, "Mr Rahane", "rahane109", "rahane109@team.com"},
};


int main(){
	int clientSocket;
	int eid,rid,n;
	
	char buf[BUFSZ]={'\0'};
    char sendbuf[BUFSZ] ={'\0'};
    char *buff = NULL;
    char *pch;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);

	//Initialize server configuration//
	readServerDetails();
	fprintf(stdout,"dibya Server IP:%s PORT:%d\n",serverip,port);

	//---- Create the socket. The three arguments are: ----//
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	
	//---- Configure settings of the server address struct ----//
	// Address family = Internet //
	serverAddr.sin_family = AF_INET;
	//Set port number, using htons function to use proper byte order //
	serverAddr.sin_port = htons(port);
	// Set IP address to localhost //
	serverAddr.sin_addr.s_addr = inet_addr(serverip);
	// Set all bits of the padding field to 0 //
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	//---- Connect the socket to the server using the address struct ----//
	addr_size = sizeof serverAddr;
	if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) )
	{
		fprintf(stderr, "dibya connect failed!.\n");
		exit(1);
	}
    fprintf(stderr, "connected to server!.\n");
	buff = (char*)malloc(sizeof(BUFSZ));

	while(1)
	{
		//---- Read the message from the server into the buffer ----//
		n = recv(clientSocket, buf, sizeof(buf), 0);
        if (n== -1)
		{
			fprintf(stderr, "connection closed.\n");
			free(buff);
			close(clientSocket);
            exit(1);
        }
		
        memcpy(buff,buf,sizeof(buf));
        pch = strtok(buff,".");
        eid = atoi(pch);

        pch = strtok(NULL,".");
        rid = atoi(pch);
        fprintf(stdout, "Request recv from server empid: %d reqid:%d\n",eid,rid);
		if (n < 0)
		{
			fprintf(stderr,"ERROR: reading data from server socket");
		}
		//memset(buff, '\0', sizeof(buff));
		lookOnEmpTable(eid, rid, sendbuf);
		
		send(clientSocket, sendbuf, sizeof(sendbuf), 0);
		fprintf(stdout, "Send data to  server: %s\n",sendbuf);

	} //end of while//

	close(clientSocket); //tear down the connection//
    free(buff);
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);
	return 0;
} //end of main//


/*********************
/* Fetch the requested data from table and send back the result
/*********************/

void lookOnEmpTable(unsigned int eid, unsigned int rid, unsigned char* buf)
{

	REQ=rid;

	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	
	if( (eid >= (INIT_EMPID + TABLESZ) || eid <= INIT_EMPID-1 ) || (REQ >=4 || REQ <=0) )
	{
		fprintf(stderr, "Data not found in server table.\n");
		strcpy(buf, SERVERERR_MSG);
		return;
	}
	eid = eid%100; //used hashing O(1) on table else needs to visit entire table O(n)

	switch(REQ)
	{
		default:
		break;
		case dependent:	  /*Employee dependent*/
		memcpy(buf, emp[eid].dname, strlen(emp[eid].dname)+1);
		break;
		
		case credential:  /*Employee credential*/
		memcpy(buf, emp[eid].creden, strlen(emp[eid].creden)+1);
		break;
		
		case mailid:	  /*Employee email id*/
		memcpy(buf, emp[eid].emailid, strlen(emp[eid].emailid)+1);
		break;

	} /* end of switch */

	return;

	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);

} /*end of lookOnEmpTable*/


/*********************
/* Read the ip and port no from configuration file
/*********************/

void readServerDetails()
{
	fprintf(stdout,"Enter function:%s()\n",__FUNCTION__);
	
	FILE *fp = NULL;
	int errnum;
	fp=fopen(CONFIG_FILE_PATH,"r");
	if (fp == NULL)
	{
		errnum = errno;
		fprintf(stderr, "Value of errno: %d\n", errno);
		fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
		exit(-1);    //exit the program if can't able to read server details
	}
	fscanf(fp,"%s %d",serverip,&port);
	fclose(fp);
	
	fprintf(stdout,"Exit function:%s()\n",__FUNCTION__);

} /*end of readServerDetails*/
