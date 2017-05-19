#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
void* thread_function(void* arg);
char message[]="THREAD_TEST";
int main()
{
	int res;
	pthread_t a_thread;
	void* thread_result;//���������߳̽���ʱ�ķ���ֵ��thread_result�������һ��ָ��
	res=pthread_create(&a_thread,NULL,thread_function,(void*)message);
	if(res!=0)
	{
		perror("pthread_create failed");
		exit(EXIT_FAILURE);
	}
	printf("�ȴ��߳̽���...\n");
	res=pthread_join(a_thread,&thread_result);//�ȴ��߳̽�����ע��&
	if(res!=0)
	{
		perror("pthread_join failed");
		exit(EXIT_FAILURE);
	}
	printf("�߳��ѽ���������ֵ: %s\n",(char*)thread_result);//����̷߳��ص���Ϣ
	printf("Message��ֵΪ�� %s\n",message);//������õ��ڴ�ռ��ֵ
	exit(SUCCESS);
}
void* thread_function(void* arg)
{
	printf("�߳������У�����Ϊ: %s\n",(char*)arg);
	sleep(3);
	strcpy(message,"�߳��޸�");//�޸Ĺ��õ��ڴ�ռ��ֵ
	pthread_exit("�߳�ִ�����");//�߳̽����������߳�ִ�����
}