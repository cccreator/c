#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<semaphore.h>
void* thread_function(void* arg);//�����̺߳���ԭ��
pthread_mutex_t work_mutex;//���廥����
#define WORK_SIZE 1024;
char work_area[WORK_SIZE];//���幫�õ��ڴ�ռ�
int time_to_exit=1;//���ڿ���ѭ��
int main()
{
	int res;
	pthread_t a_thread;
	void* thread_result;
	res=pthread_mutex_init(work_mutex,NULL);//��������ʼ��������
	if(res!=0)
	{
		perror("pthread_mutex_init failed");
		exit(EXIT_FAILURE);
	}
	res=pthread_create(&a_thread,NULL,thread_function,NULL);
	if(res!=0)
	{
		perror("pthread_create failed");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_lock(&work_mutex);//�ѻ�����mutex��������ȷ��ͬһʱ��ֻ�и��߳̿��Է���word_area�е�����
	printf("������Ҫ���͵���Ϣ������'end'�˳�\n");
	while(time_to_exit)
	{
		fgets(work_area,WORK_SIZE,stdin);//����������Ϣ
		pthread_mutex_unlock(&work_mutex);//�ѻ��������������������߳̿��Է���work_area�е�����
		while(1)
		{
			pthread_mutex_lock(&work_mutex);
			if(work_area[0]!='\0')//�жϹ����ڴ�ռ��Ƿ�Ϊ��
			{
				pthread_mutex_unlock(&work_mutex);
				sleep(1);
			}
			else
			{
				break;
			}
		}
	}
	pthread_mutex_unlock(&work_mutex);
	printf("\n �ȴ��߳̽���...\n");
	res=pthread_join(a_thread,&thread_result);
	printf("�߳̽���\n");
	pthread_mutex_destroy(&work_mutex);//���������
	exit(EXIT_SUCCESS);
}
void* thread_function(void* arg)
{
	sleep(1);
	pthread_mutex_lock(&work_mutex);
	while(strncmp("end",work_area,3)!=0)//�ж��յ�����Ϣ�Ƿ�Ϊend
	{
		printf("�յ�%d���ַ�\n",strlen(work_area)-1);
		work_area[0]='\0';//�������ռ����
		pthread_mutex_lock(&work_mutex);
		sleep(1);
		pthread_mutex_lock(&work_mutex);
		while(work_area[0]=='\0')//�жϹ����ռ��Ƿ�Ϊ��
		{
			pthread_mutex_unlock(&work_mutex);
			sleep(1);
			pthread_mutex_lock(&work_mutex);
		}
	}
	time_to_exit=0;//��ѭ��������־��Ϊ0
	work_area[0]='\0';//��������ռ�
	pthread_mutex_unlock(&work_mutex);
	pthread_exit(0);//�����߳�
}