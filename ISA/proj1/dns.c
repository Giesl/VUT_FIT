/*
* FILENAME :        dns.c             
*
* DESCRIPTION :		Simple DNS resolver.
* AUTHOR :    		xgiesl00
* DATE :    		08 Oct 19
*     
*/


//includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>


//Define variables
#define PORT 53
#define DNS_SERVER "8.8.8.8"
#define BUFFER_SIZE 65536
#define TYPE_VALUE 1 	//IPv4
#define CLASS_VALUE 1	//internet

//define structures
//DNS header structure RFC
typedef struct DNS_HEADER
{
    unsigned short id; // identification number
 
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag
 
    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available
 
    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
}DNS_HEADER;



typedef struct RES_REC
{
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short rdLength;

}RES_REC;


//RESULT struct create resource record format
//RFC 1035 4.1.3
typedef struct RESULT
{
	unsigned char *name;
	RES_REC *res;
	unsigned char *rdata;
}RESULT;


//RFC 1035 4.1.2
typedef struct QUESTION
{
	unsigned char *name;
	unsigned short qtype;
	unsigned short qclass;
}QUESTION;
//functions declarations
void get_qname(unsigned char* text, unsigned char* dst);

int main()
{
	printf("hello\n");
	unsigned char* hostname = "www.neco.cz";
	unsigned char qname[30];
	get_qname(hostname,qname);
	printf("%s\n",qname);
}



void get_dns_answer(char *target)
{


	unsigned char buffer[BUFFER_SIZE],*qname;
	//create socket
	int s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
 	struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(DNS_SERVER); //dns servers
 	

 	DNS_HEADER *dns = NULL;
 	QUESTION *qinfo = NULL;


 	dns = (DNS_HEADER *)&buffer;

 	dns->id = (unsigned short) htons(getpid());
 	dns->qr = 0;
 	dns->opcode = 0;
 	dns->aa = 0;
 	dns->tc = 0;
 	dns->rd = 1;
 	//jeste zmenit na zaklade parametru
 	dns->ra = 0; //recursive
 	//
 	dns->z = 0;
 	dns->ad = 0;
 	dns->cd = 0;
 	dns->rcode = 0;
 	dns->q_count = htons(1);
 	dns->ans_count = 0;
 	dns->auth_count = 0;
 	dns->add_count = 0;

 	//insert qname after header
 	qname = (unsigned char *)&buffer[sizeof(DNS_HEADER)];
 	//translate target 
 	get_qname(target,qname);
 	//apend after qname
 	qinfo = (QUESTION*)&buffer[(sizeof(DNS_HEADER) + strlen((const char*)qname) + 1)];
 	//set up QUESTION
 	qinfo->qtype = TYPE_VALUE;
 	qinfo->qclass = CLASS_VALUE;

}
void get_qname(unsigned char* text, unsigned char* dst)
{
	char str[20];
	strcpy(str,text);
	int init_size = strlen(str);
	char delim[] = ".";
	char *ptr = strtok(str, delim);


	while(ptr != NULL)
	{
		*dst = (int) strlen(ptr) + '0';
		*dst++;
		for(int i = 0; i< strlen(ptr);i++)
		{
			*dst = ptr[i];
			*dst++;
		}
		ptr = strtok(NULL, delim);
	}
	*dst++;
	*dst = '\0';
}

