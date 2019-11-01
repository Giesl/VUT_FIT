/*
* FILENAME :        dns.c             
*
* DESCRIPTION :		Simple DNS resolver.
* AUTHOR :    		xgiesl00
* DATE :    		08 Oct 19
*     
*/


//Header Files
#include	<stdio.h> 		
#include	<string.h>    	
#include	<stdlib.h>    
#include	<sys/socket.h> 
#include	<arpa/inet.h> 
#include	<netinet/in.h>
#include	<unistd.h>    
#include	<stdbool.h>	
#include    <netdb.h>


//Define variables
#define PORT 53
#define BUFFER_SIZE 65536		
#define TIMEOUT_SEC 5
#define A_TYPE 1            //IPv4
#define CLASS_IN 1          //internet
#define CLASS_CNAME 5
#define PTR_TYPE 12         //reversed query
#define AAAA_TYPE 28        //IPv6
 
 
//DNS header structure
struct DNS_HEADER
{
    unsigned short id       :16; // identification number
 
    unsigned char rd        :1; // recursion desired
    unsigned char tc        :1; // truncated message
    unsigned char aa        :1; // authoritive answer
    unsigned char opcode    :4; // purpose of message
    unsigned char qr        :1; // query/response flag
 
    unsigned char rcode     :4; // response code
    unsigned char cd        :1; // checking disabled
    unsigned char ad        :1; // authenticated data
    unsigned char z         :1; // its z! reserved
    unsigned char ra        :1; // recursion available
 
    unsigned short q_count;     // number of question entries
    unsigned short ans_count;   // number of answer entries
    unsigned short auth_count;  // number of authority entries
    unsigned short add_count;   // number of resource entries
};
 
//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};

typedef struct ipv6_part{
   unsigned char part :4;
} ipv6_part;

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


typedef enum
{
    PORT_ERROR,
    TIMEOUT,
    RECEIVING_ERROR,
    GETADDRINFO_ERROR,
    SERVER_IP_ERROR,
    QCLASS_ERROR,
    TYPE_ERROR,
    OTHER_ERROR,
}error_codes;

//functions declarations
void get_qname(unsigned char* text, unsigned char* dst);
void get_dns_answer(unsigned char *target, bool recursive, bool ipv6, unsigned char *server, int port,bool reversed);
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host);
void ngethostbyname(unsigned char *host , int query_type);
void get_name(unsigned char* data,unsigned char* dest, unsigned int* length, unsigned char* buffer);
u_char* ReadName(unsigned char* reader,unsigned char* buffer,int* count);
char* get_type_name(int num);
char* get_class_name(int num);
char* lookup_host (const char *host,bool ipv6);


int main(int argc, char *argv[])
{
	

	unsigned char hostname[100],*server;
    int opt,port = 53;
    bool recursive=false,ipv6=false,reverse = false;
    long lnum;
    char *end;


    
	while((opt = getopt(argc, argv, ":s:p:r6x")) != -1)
    {
        switch(opt)
        {
            case 'r': recursive = true;
                break;

            case 's': server = optarg;
                break;

            case '6': ipv6 = true;
                break;

            case 'p': port = strtol(optarg, &end, 10);
                break;

            case 'x': reverse = true;
                break;
        }
    }

    //check port 
    if((end != NULL && strlen(end)>0) || port < 1 || port > 65535)
    {
      fprintf(stderr,"ERROR with given port\n");
      exit(PORT_ERROR);
    }

	//get hostname from arguments 
	strcpy(hostname,argv[argc-1]);
    char* server_ip = lookup_host(server,ipv6);

    if(server_ip == NULL)
    {
        fprintf(stderr, "Error with getting server IP address\n");
        exit(SERVER_IP_ERROR);
    }

	get_dns_answer(hostname,recursive,ipv6,server_ip,port,reverse);
	
	return 0;
}




char* get_type_name(int num)
{
    char* type = NULL;
    switch(num)
        {
            case 1: type    = "A";break;
            case 2: type    = "NS";break;
            case 3: type    = "MD";break;
            case 4: type    = "MF";break;
            case 5: type    = "CNAME";break;
            case 6: type    = "SOA";break;
            case 7: type    = "MB";break;
            case 8: type    = "MG";break;
            case 9: type    = "MR";break;
            case 10: type   = "NULL";break;
            case 11: type   = "WKS";break;
            case 12: type   = "PTR";break;
            case 13: type   = "HINFO";break;
            case 14: type   = "MINFO";break;
            case 15: type   = "MX";break;
            case 16: type   = "TXT";break;
            case 28: type   = "AAAA";break;
        }
    if(type == NULL)
    {
        fprintf(stderr, "UNDEFINED TYPE\n");
        exit(TYPE_ERROR);
    }
    return type;
}

char* get_class_name(int num)
{   
    char* qclass = NULL;
    switch(num)
        {
            case 1: qclass = "IN";break;
            case 2: qclass = "CS";break;
            case 3: qclass = "CH";break;
            case 4: qclass = "HS";break;
            case 5: qclass = "CNAME";break;
        }
    if(qclass == NULL)
    {
        fprintf(stderr, "UNDEFINED CLASS\n");
        exit(QCLASS_ERROR);
    }
    return qclass;
}

void get_dns_answer(unsigned char *target, bool recursive, bool ipv6, unsigned char *server,int port,bool reversed)
{


	unsigned char buffer[BUFFER_SIZE],*qname,*data;
	//create socket
	int sfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
    //set timeout to 5s
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);



 	struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = inet_addr(server); //dns servers
 	

 	struct DNS_HEADER *dns = NULL;
 	struct QUESTION *qinfo = NULL;

	
 	dns = (struct DNS_HEADER *)&buffer;

 	dns->id = (unsigned short) htons(getpid());
    dns->qr = 0;
    dns->opcode = 0;
    dns->aa = 0;
    dns->tc = 0; 

    dns->rd = 0;
    if(recursive) dns->rd = 1; 

    dns->ra = 0; 
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = (unsigned short) htons(1);
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;




    if(reversed)
    {
        printf("REVERSED QUERY\n");
        if(!ipv6)
        {
            char reversed_ip[INET_ADDRSTRLEN];

            in_addr_t addr;

            /* Get the textual address into binary format */
            inet_pton(AF_INET, target, &addr);
        
            /* Reverse the bytes in the binary address */
            addr =
                ((addr & 0xff000000) >> 24) |
                ((addr & 0x00ff0000) >>  8) |
                ((addr & 0x0000ff00) <<  8) |
                ((addr & 0x000000ff) << 24);
        
            /* And lastly get a textual representation back again */
            inet_ntop(AF_INET, &addr, reversed_ip, sizeof(reversed_ip));
            strcat(reversed_ip,".in-addr.arpa");
            target = reversed_ip;
        }
        else
        {
            char reversed_ip[INET6_ADDRSTRLEN];
            struct in6_addr addr;
            inet_pton(AF_INET6,target,&addr);
            char str2[48];

            for(int i = 15;i>=0;i--)
            {
                //printf("%hu",addr.s6_addr[i]);
                char s = ((addr.s6_addr[i] & 0xf0));
                //printf("->%hu->",s);
                char s2 = s >> 4;
                //printf("%hu\n",s2);
                
                //printf("%hu",addr.s6_addr[i]);
                char e = ((addr.s6_addr[i] & 0x0f));
                //printf("->%hu->",s);

                //printf("%x.",s2);
                //printf("%hu\n",s2);
                sprintf(reversed_ip,"%s%x.%x.",reversed_ip,e,s2);
            }
            
            strcat(reversed_ip,"ip6.arpa");
            target = reversed_ip;
        }

        printf("REVERSED IP:%s\n", target);
       
 	}

    //insert qname after header
    qname = (unsigned char *)&buffer[sizeof(struct DNS_HEADER)];
    //translate target 

    get_qname(target,qname);
   
    
    
    printf("qname:%s\n",qname);
    printf("strlen:%d\n",strlen((const char*)qname));

    printf("HERE\n");
    //apend after qname
    qinfo = (struct QUESTION*)&buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
    //set up QUESTION
    
    if(reversed)
    {
        qinfo->qtype    = htons(PTR_TYPE);
        
    }
    else if(ipv6)
    {
        qinfo->qtype    = htons(AAAA_TYPE);
    }
    else
    {
        qinfo->qtype    = htons(A_TYPE);
    }
    qinfo->qclass   = htons(CLASS_IN);


    printf("QTYPE:%d\n",ntohs(qinfo->qtype));
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
        exit(RECEIVING_ERROR);

 	}
 	
 	//initalize pointers to dns response sections
 	dns = (struct DNS_HEADER*) buffer;
 	//data = &buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1 ) + sizeof(struct QUESTION)];
    data = buffer + sizeof(struct DNS_HEADER);
    //Question section

    printf("Authoritative: %d, Recursive: %d, Truncated: %d\n",dns->aa,dns->rd,dns->tc);

    int stop = 0;
    printf("Question section (%d)\n", ntohs(dns->q_count));
    for(int i = 0;i<ntohs(dns->q_count);i++)
    {
        int length;
        unsigned char name[100];
        
        get_name(data,name,&length,buffer);
        data = data + length;
        printf("TEST\n");
        struct QUESTION *q = (struct QUESTION*)(data);
        data = (char*) data + sizeof(struct QUESTION);
        char *type = get_type_name(ntohs(q->qtype));
        char *qclass = get_class_name(ntohs(q->qclass));

        printf("%s, %s, %s\n",name,type,qclass);
    }

    //Answer section
    printf("Answer section (%d)\n", ntohs(dns->ans_count));
    for(int i = 0;i<ntohs(dns->ans_count);i++)
    {
    	
    	//get name
    	int length;
    	unsigned char name[100];
    	get_name(data,name,&length,buffer);
    	data = (void*) data + length;
    	


    	//get resources
    	struct ANSWER answer;
    	answer.name = name;
    	answer.resource = (struct REC_DATA*)(data);

    	data = data + 10;//sizeof(struct REC_DATA);
    	
    	//1 	-> IPv4
    	//28 	-> IPv6
    	char* msg = "";
        int type = ntohs(answer.resource->type);
        
    	if(type == A_TYPE || type == AAAA_TYPE)
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

    		char *type = get_type_name(ntohs(answer.resource->type));
	        char *qclass = get_class_name(ntohs(answer.resource->_class));
	      


	        if(ntohs(answer.resource->_class) == CLASS_IN)
	        {
                
                if(ntohs(answer.resource->type) == A_TYPE)
                {
	            long *p;
	            struct sockaddr_in a;
	            p=(long*)answer.data;
	            a.sin_addr.s_addr=(*p);
	            msg = inet_ntoa(a.sin_addr);
                }
                else if(ntohs(answer.resource->type) == AAAA_TYPE)
                {
                    __int128 *p;
                    struct sockaddr_in a;
                    p=(__int128*) answer.data;
                    char str[INET6_ADDRSTRLEN];
                    if (inet_ntop(AF_INET6, p, str, INET6_ADDRSTRLEN) == NULL)
                    {
                        perror("inet_ntop");
                        exit(EXIT_FAILURE);
                    }
                    msg = str;
                }

	        }
	        else if (ntohs(answer.resource->_class) == CLASS_CNAME)
	        {
                unsigned char name[100];
                get_name(answer.data,name,&length,buffer);
	        	msg = name;
	        }
	
	    	printf("%s, %s, %s, %d, %s \n",name,type,qclass,ntohl(answer.resource->ttl),msg);
	    }

	
    }

    //Authority section
    printf("Authority section (%d)\n", ntohs(dns->auth_count));
    for(int i = 0; i < ntohs(dns->auth_count);i++)
    {
    	//get name
    	int length;
    	unsigned char name[100];
    	get_name(data,name,&length,buffer);
    	data = (void*) data + length;
    	
    	struct ANSWER answer;
    	answer.name = name;
    	answer.resource = (struct REC_DATA*)(data);


    	data = data + 10;//sizeof(struct REC_DATA);

        
        //seek the pointer
        
        char *type = get_type_name(ntohs(answer.resource->type));
        char *qclass = get_class_name(ntohs(answer.resource->_class));
        printf("%s, %s, %s, %d, ",name,type,qclass,ntohl(answer.resource->ttl));

   		for(int j = 0; j<ntohs(answer.resource->data_len);j++)
   		{
            printf("%02hhX ",data[j]);
   			
   		}
        printf("\n");
   		data = data + ntohs(answer.resource->data_len);
        
    
        

    	
	    

    }

    //Adition section
    printf("Aditional section (%d)\n",ntohs(dns->add_count));
    for(int i = 0; i < ntohs(dns->add_count);i++)
    {
    	//get name
    	int length;
    	unsigned char name[100];
    	get_name(data,name,&length,buffer);
    	data = (void*) data + length;
    	
    	struct ANSWER answer;
    	answer.name = name;
    	answer.resource = (struct REC_DATA*)(data);


    	data = data + 10;//sizeof(struct REC_DATA);


   		//copy data
        char *type = get_type_name(ntohs(answer.resource->type));
        char *qclass = get_class_name(ntohs(answer.resource->_class));
        printf("%s, %s, %s, %d, ",name,type,qclass,ntohl(answer.resource->ttl));
   		for(int j = 0; j<ntohs(answer.resource->data_len);j++)
   		{
   			printf("%02hhX",data[j]);
   			
   		}
        printf("\n");
   		//add end of string to data
   		//seek the pointer
   		data = data + ntohs(answer.resource->data_len);
   		

    	
	    
    }


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

char* lookup_host (const char *host,bool ipv6)
{
  struct addrinfo hints, *res;
  int errcode;
  char *addrstr = malloc(sizeof(char)*100);
  void *ptr;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  errcode = getaddrinfo(host, NULL, &hints, &res);
  if (errcode != 0)
    {
      perror ("getaddrinfo");
      exit(GETADDRINFO_ERROR);
    }

  //printf ("Host: %s\n", host);
  while (res)
    {
      inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

      switch (res->ai_family)
        {
        case AF_INET:
          ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
          break;
        case AF_INET6:
          ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
          break;
        }
      inet_ntop (res->ai_family, ptr, addrstr, 100);
      //printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4, addrstr, res->ai_canonname);
        
      return addrstr;
      res = res->ai_next;
    }
  free(addrstr);
  return NULL;
}



//transform hostname to qname
void get_qname(unsigned char* text, unsigned char* dst)
{

	char str[INET6_ADDRSTRLEN];
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


