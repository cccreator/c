#include<iostream>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>

using namespace std;

#define MAXLINE 5
#define OPEN_MAX 100
#define LISTENQ 20
#define SERV_PORT 5000
#define INFTIM 1000

void setnonblocking(int sock)
{
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0)
	{
		perror("fcntl(sock,GETFL)");
		exit(1);
	}
	opts=opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<))
	{
		perror("fcntl(sock,SETFL,opts)");
		exit(1����
	}
}
int main(int argc,char* argv[])
{
	int i,maxi,listenfd,connfd,sockfd,epfd,nfds,portnumber;
	ssize_t n;
	char line[MAXLINE];
	socklen_t clilen;
	if(2==argc)
	{
		if((portnumber=atoi(argv[1]))<0)
		{
			printf(stderr,"Usage:%s portnumber/a/n",argv[0]);
			return 1;
		}
	}
	else
	{
		fprintf(stderr,"Usage:%s portnumber/a/n",argv[0]);
		return 1;
	}
	//����epoll_event�ṹ��ı�����ev����ע��ʱ�䣬events[20]���ڻش�Ҫ������¼�
	struct epoll_event ev,events[20];
	//�������ڴ���accept��epollר�õ��ļ�������
	epfd=epoll_create(256);
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	setnonblocking(listenfd);//��socket����Ϊ��������ʽ
	
	ev.data.fd=listenfd;//����Ҫ������¼���ص��ļ�������
	
	ev.events=EPOLLIN|EPOLLET;//����Ҫ������¼�����

	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);//ע��epoll�¼�
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family=AF_INET;
	char* local_addr="127.0.0.1";
	inet_aton(local_addr,&(serveraddr.sin_addr));
	serveraddr.sin_port=htons(portnumber);
	bind(listenfd,(sockaddr*)&serveraddr,sizeof(serveraddr));
	listen(listenfd,LISTENQ);
	maxi=0;
	for( ; ; )
	{
		nfds=epoll_wait(epfd,events,20500);//�ȴ�epoll�¼��ķ���
		
		for(i=0;i<nfds;++i)//�����������ĵ������¼�
		{
			if(events[i].data.fd==listenfd)//����¼�⵽һ��SOCKET�û�����
				//���˰󶨵�SOCKET�˿ڣ������µ�����
			{
				connfd=accept(listenfd,(sockaddr*)&clientaddr,&clilen);
				if(connfd<0)
				{
					perror("connfd<0");
					exit(1);
				}
				//setnonblocking(connfd);
				char* str=inet_ntoa(clientaddr.sin_addr);
				cout<<"accept a connection from"<<str<<endl;

				ev.data.fd=connfd;//���ڶ��������ļ�������

				ev.events=EPOLLIN|EPOLLET;//��������ע��Ķ������¼�

				epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);//ע��ev
			}
			else if(events[i].events&EPOLLIN)
			//������Ѿ����ӵ��û��������յ�����û��ô���ж��롣
			{
				cout<<"EPOLLIN"<<endl;
				if((sockfd=events[i].data.fd)<0)
					continue;
				if((n=read(sockfd,line,MAXLINE))<0)
				{
					if(errno==ECONNRESET)
					{
						close(sockfd);
						events[i]data.fd=-1;
					}
					else
						std::cout<<"readline error:"<<std::endl;
				}
			else if(n==0)
			{
				close(sockfd);
				wvents[i]data.fd=-1;
			}
			line[n]='/0';
			cout<<"read"<<line<<endl;

			ev.data.fd=sockfd;//��������д�������ļ�������

			ev.events=EPOLLOUT|EPOLLET;//��������ע���д�����¼�
			//�޸�sockfd��Ҫ������¼�ΪEPOLLOUT
			//epoll_ctr(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
			else if(events[i],events&EPOLLOUT)
			{
				sockfd=events[i].data.fd;
				write(sockfd,line,n);//�������ڶ��������ļ�������

				ev.data.fd=sockfd;//��������ע��Ķ������¼�

				ev.events=EPOLLIN|EPOLLET;//�޸�sockfd��Ҫ������¼�ΪEPOLLIN

				epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
		}
	}
	return 0;
}