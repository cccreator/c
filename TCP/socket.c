Server:


#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<stdio.h>

void main()
{
	//Э�̰汾
	WORD wVersionRequested;
	int err;

	wVersionRequested=MAKEWORD(1,1);//0x0101
	err=WSAStartup(wVersionRequested,&wsaData);
	if(err!=0)
	{
		return 0;
	}
	if(LOBYTE(wsaData.wVersion)!=1||HIBYTE(wsaData.wVersion)!=1)
	{
		WSACLeanup();
		return 0;
	}

	//����socket
	SOCKET sockSvr = socket(AF_INET,SOCK_STREAM,0);

	//����IP��ַ�Ͷ˿�
	SOCKADDR_IN addrSvr;
	addrSvr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);//�����׽��ֵ�ַ
	addrSvr.sin_family=AF_INET;//�����׽��ֵ�ַ�е���
	addrSvr.sin_port=htons(6000);//�����׽��ֶ˿�

	//�󶨶˿ڣ�����
	bind(sockSvr,(SOCKADDR*)&addrSvr,sizeof(SOCKADDR));
	listen(sockSvr,5);

	sockaddr_in addrClient;
	int len=sizeof(sockaddr);

	while(true)
	{
		//�������������һ���ͻ�Socket����
		SOCKET sockConn = accept(sockSvr,(sockaddr*)&addrClient,&len);
		char sendbuffer[128];
		sprintf(sendbuffer,"Welcom %s!",inet_ntoa(addrClient.sin_addr));

		//��ͻ�Socket��������
		send(sockConn,sendbuffer,strlen(sendbuffer)+1,0);
		char recvbuffer[128];

		//�ӿͻ�Socket��������
		recv(sockConn,recvbuffer,128,0);
		printf("%s/n",recvbuffer);

		//�ر�Socket
		closesocket(sockConn);
	}
	closesocket(sockSvr);

	//�ͷ�Winsock��Դ
	WSACleanup();
	return 0;
}


client:


#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<stdio.h>

void main()
{
	//Э�̰汾
	WORD wVersionRequested = MAKEWORD(1,1);//0x0101
	err=WSAStartup(wVersionRequested,&wsaData);

	if(err!=0)
	{
		return 0;
	}
	if(LOBYTE(wsaData.wVersion)!=1||HIBYTE(wsaData.wVersion)!=1)
	{
		WSACleanup();
		return 0;
	}

	//����������������׽���
	SOCKET sock=socket(AF_INET,SOCK_STREAM,0);

	//������ַ��Ϣ
	SOCKADDR_IN hostAddr;
	hostAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	hostAddr.sin_family=AF_INET;
	hostAddr.sin_port=htons(6000);

	//���ӷ�����
	connect(sock,(sockaddr*)&hostAddr,sizeof(sockaddr));

	char revBuf[128];

	//�ӷ������������
	recv(sock,recvBuf,128,0);
	printf("%s/n",revBuf);

	//���������������
	send(sock,"Hello Host!",12,0);
	closesocket(sock);
	return 0;
}