/*
* FILENAME :        dns.c             
*
* DESCRIPTION :		Simple DNS resolver.
* AUTHOR :    		xgiesl00
* DATE :    		08 Oct 19
*     
*/


//Header Files
#include	<stdio.h> //printf
#include	<string.h>    //strlen
#include	<stdlib.h>    //malloc
#include	<sys/socket.h>    //you know what this is for
#include	<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include	<netinet/in.h>
#include	<unistd.h>    //getpid
#include	<stdbool.h>


//Define variables
#define PORT 53
#define DNS_SERVER "8.8.8.8"
#define BUFFER_SIZE 65536
#define TYPE_VALUE 1 	//IPv4
#define CLASS_VALUE 1	//internet
 
 
//DNS header structure
struct DNS_HEADER
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
};
 
//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};
 
//Constant sized fields of the resource record structure
struct REC_DATA
{
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short data_len;

};

struct ANSWER
{
	unsigned char* name;
	struct REC_DATA *resource;
	unsigned char *data;
};
 
//Structure of a Query
typedef struct
{
    unsigned char *name;
    struct QUESTION *ques;
} QUERY;

//functions declarations
void get_qname(unsigned char* text, unsigned char* dst);
void get_dns_answer(unsigned char *target);
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host);
void ngethostbyname(unsigned char *host , int query_type);
void get_name(unsigned char* data,unsigned char* dest, unsigned int* length, unsigned char* buffer);
u_char* ReadName(unsigned char* reader,unsigned char* buffer,int* count);

int main(int argc, char *argv[])
{
	printf("START OF PROGRAM\n");
	//unsigned char* hostname = "www.google.com";
	//get_dns_answer(hostname);
	

	unsigned char hostname[100];
	unsigned char qname[30];
	
	//get hostname from arguments 
	strcpy(hostname,argv[argc-1]);
	get_dns_answer(hostname);
	
	
	return 0;
}



void get_dns_answer(unsigned char *target)
{


	unsigned char buffer[BUFFER_SIZE],*qname,*data;
	//create socket
	int sfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
 	struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8"); //dns servers
 	

 	struct DNS_HEADER *dns = NULL;
 	struct QUESTION *qinfo = NULL;

	
 	dns = (struct DNS_HEADER *)&buffer;

 	dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    //--TODO--
    //nastavit rekurzi na základně parametrů
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;




 	//insert qname after header
 	qname = (unsigned char *)&buffer[sizeof(struct DNS_HEADER)];
 	//translate target 
 	get_qname(target,qname);
 	//apend after qname
 	qinfo = (struct QUESTION*)&buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
 	//set up QUESTION
 	qinfo->qtype = htons(TYPE_VALUE);
 	qinfo->qclass = htons(CLASS_VALUE);

 	
 	if( sendto(sfd,(char*)buffer,sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),0,(struct sockaddr*)&dest,sizeof(dest)) < 0)
 	{
 		perror("Error with sending datagram");
 	}
 	
 	//now wait for answer from server
 	int i = sizeof dest;

 	//read answer to buffer
 	int a = recvfrom(sfd,(char*)buffer, BUFFER_SIZE, 0, (struct sockaddr*)&dest, (socklen_t*)&i );
 	printf("Number of received bytes: %d \n", a);
 	if( a < 0 )
 	{
 		perror("Error with receiving answer\n");

 	}
 	
 	//initalize pointers to dns response sections
 	dns = (struct DNS_HEADER*) buffer;
 	//data = &buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1 ) + sizeof(struct QUESTION)];
    data = buffer + sizeof(struct DNS_HEADER);
    //Question section
    int stop = 0;
    printf("Question section (%d)\n", ntohs(dns->q_count));
    for(int i = 0;i<ntohs(dns->q_count);i++)
    {
        int length;
        unsigned char name[100];
        get_name(data,name,&length,buffer);
        data = data + length;
        //printf("\nNAME:%s",name);
        //printf("\nLENGTH:%d",length); 
        struct QUESTION *q = (struct QUESTION*)(data);
        //printf("\nTYPE:%d\n",ntohs(q->qtype));
        //printf("CLASS:%d->",ntohs(q->qclass));

        data = (char*) data + sizeof(struct QUESTION);

        char *type = "AAA";
        if(ntohs(q->qtype) == 1)
        {
            type = "A";
        }
        char *qclass = "ERROR";
        if(ntohs(q->qclass) == 1)
        {
        	qclass = "IN";
        }


        //--TODO--
        printf("%s, %s, %s\n",name,type,qclass);
    }

    //Answer section
    printf("Answer section (%d)", ntohs(dns->ans_count));
    for(int i = 0;i<ntohs(dns->ans_count);i++)
    {
    	
    	//get name
    	int length;
    	unsigned char name[100];
    	get_name(data,name,&length,buffer);
    	data = data + length;

    	//get resources
    	struct ANSWER answer;
    	answer.name = name;
    	answer.resource = (struct REC_DATA*)(data);
    	data += sizeof(struct REC_DATA);

    	printf("\nNAME:%s",answer.name);
        printf("\nNAME LENGTH:%d",length);
    	printf("\nTTL:%d\n",answer.resource->ttl);
    	printf("\nDATA LENGTH:%d",answer.resource->data_len);
        printf("\nTYPE:%d\n",ntohs(answer.resource->type));
    	
    	//1 	-> IPv4
    	//28 	-> IPv6
        
        int type = ntohs(answer.resource->type);
        
    	printf("type:%d\n",type);
    	
    	if(type == 1)
    	{
    		answer.data = (unsigned char*)malloc(answer.resource->data_len);
    		//copy data
    		for(int j = 0; j<ntohs(answer.resource->data_len);j++)
    		{
    			answer.data[j]=data[j];
    		}
    		//add end of string to data
    		answer.data[ntohs(answer.resource->data_len)] = '\0';
    		//seek the pointer
    		data = data + ntohs(answer.resource->data_len);
    		char *type = "AAA";
	        if(ntohs(answer.resource->type) == 1)
	        {
	            type = "A";
	        }
	        char *qclass = "ERROR";
	        if(ntohs(answer.resource->_class) == 1)
	        {
	        	qclass = "IN";
	        }
	
	    		printf("%s, %s, %s, %d, %s \n",name,type,qclass,answer.resource->ttl,"NECO");
	    	}


	   	else if (type == 28)
	   	{
	
	   	}
	   	else
	   	{
	
	    }

    	printf("END\n");
    	exit(0);

    }

    //Authority section
    printf("\nAuthority section (%d)", ntohs(dns->auth_count));


    //Adition section
    printf("\nAditional section (%d)",ntohs(dns->add_count));



    printf("\n");
}


void get_name(unsigned char* data,unsigned char* dest, unsigned int* length, unsigned char* buffer)
{	
	int pos = 0;
	*length = 1;
	bool jumped = false;


	//start of string
	dest[0]='\0';
	while(*data!=0)
	{
		if(*data<192)
		{
			
			dest[pos++] = *data;
			

		}
		//need to jump
		else
		{
			//referenct to http://www.tcpipguide.com/free/t_DNSNameNotationandMessageCompressionTechnique-2.htm
			int offset = (*data)*256 + *(data+1) - 49152;
			data = buffer + offset -1;
			jumped = true;

		}

        if(jumped == false)
        {
            *length = *length + 1;
        }
		data = data + 1;
	}
	//end of string
	dest[pos]='\0';
	if(jumped)
	{
		*length = *length + 1;
	}
	int num;
	int i;
	for(i = 0;i<(int)strlen((const char*)dest);i++)
	{
		num=dest[i];
		for(int j = 0;j<(int)num;j++)
		{
			dest[i]=dest[i+1];
			i++;
		}
		dest[i]='.';
	}
	dest[i-1]='\0';


}

//transform hostname to qname
void get_qname(unsigned char* text, unsigned char* dst)
{

	char str[20];
	strcpy(str,text);
	int init_size = strlen(str);
	char delim[] = ".";
	char *ptr = strtok(str, delim);


	while(ptr != NULL)
	{
		*dst = (int) strlen(ptr);
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

