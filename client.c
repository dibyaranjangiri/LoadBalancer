#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define FPATH "inputfile.txt"
#define BUFFSZ 100 

int main()
{
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  int empid,reqid,n;
  int clientSocket;
  char buf[BUFFSZ]={'0'};
  char recvbuffer[BUFFSZ]={'0'};
  FILE *fp=NULL;

  fprintf(stderr, "Enter Function: %s()\n",__FUNCTION__);

  fp = fopen(FPATH, "r");
  if (fp == NULL)
  {
    fprintf(stdout, "Can't open input file inputfile.txt \n");
    exit(1);
  }


  /* Create the socket */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);

  /* Configure settings of the server address struct */
  /* Address family = Internet */
  serverAddr.sin_family = AF_INET;

  /* Set port number, using htons function to use proper byte order */
  serverAddr.sin_port = htons(9801);

  /* Set IP address to localhost */
  serverAddr.sin_addr.s_addr = inet_addr("10.75.11.25");

  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  /* Connect the socket to the server using the address struct */
  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	
	
	while( fscanf(fp,"%d%d",&empid,&reqid) != EOF )
	{
		sprintf(buf,"%d.%d",empid,reqid);
		
		n = send(clientSocket, buf, sizeof(buf), 0);
		if (n == -1)
		{
			fprintf(stderr,"ERROR: sending request to server socket");
			fclose(fp);
			exit(-1);			
		}

	sleep(1);
		
		memset(recvbuffer, '\0', sizeof(recvbuffer));
		n = read(clientSocket, recvbuffer, sizeof(recvbuffer)-1 );
		if (n < 0)
		{
			fprintf(stderr,"ERROR: reading data from server socket");
			close(clientSocket);
			fclose(fp);
			exit(1);
		}

		switch(reqid)
		{
			default:
			break;
			case 1:
			fprintf(stdout,"Employee No: %u has dependent name: %s\n",empid,recvbuffer);
			break;
			case 2:
			fprintf(stdout,"Employee No: %u has credential: %s\n",empid,recvbuffer);
			break;
			case 3:
			fprintf(stdout,"Employee No: %u has email id: %s\n",empid,recvbuffer);
			break;
		}
		
    } /*end of while loop*/
	
    close(clientSocket);
    fclose(fp);
    fprintf(stdout, "Exit Function: %s()\n",__FUNCTION__);
    return 0;
}
