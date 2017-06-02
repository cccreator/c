
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <mysql/mysql.h>

#define BUFSIZE 1024

/*
 printf("AAAAAAAAA");//���ʱ��printf���������ַ����ŵ�������������棬ֱ������\n�Ż�����������������������

 memset(name, 0, sizeof(name));
 read(STDIN_FILENO, name, sizeof(name));//�ȴ� �û�����Ҫɾ��������,�����û��������"������"
 name[strlen(name) - 1] = 0;//���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'
 */

MYSQL mysql, *connection;

void deletename(char *SQL)
{
	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ�ɵ�������>:");
	write(STDOUT_FILENO, SQL, strlen(SQL)); //��仰�����printf�����ģ�����д�Ͳ���Ҫ\nҲ������Ļ�����
	char name[1024];
	memset(name, 0, sizeof(name));
	read(STDIN_FILENO, name, sizeof(name)); //�ȴ� �û�����Ҫɾ��������,�����û��������"������"
	name[strlen(name) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "DELETE FROM table1 WHERE name = '%s'", name); //
	//SQL = DELETE FROM table1 WHERE name = '������'
	printf("'%s'\n", SQL);
}

void insertname(char *SQL)
{
	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ���������>:");
	write(STDOUT_FILENO, SQL, BUFSIZE); //��仰�����printf�����ģ�����д�Ͳ���Ҫ\nҲ������Ļ�����
	char name[1024];
	memset(name, 0, sizeof(name));
	read(STDIN_FILENO, name, sizeof(name)); //�ȴ� �û�����Ҫ���������,�����û��������"������"
	name[strlen(name) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ������Ա�>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char sex[1024];
	memset(sex, 0, sizeof(sex));
	read(STDIN_FILENO, sex, sizeof(sex)); //�ȴ� �û�����Ҫ������Ա�,
	sex[strlen(sex) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ���������>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char age[1024];
	memset(age, 0, sizeof(age));
	read(STDIN_FILENO, age, sizeof(age)); //�ȴ� �û�����Ҫ���������
	age[strlen(age) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ����İ༶>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char classes[1024];
	memset(classes, 0, sizeof(classes));
	read(STDIN_FILENO, classes, sizeof(classes)); //�ȴ� �û�����Ҫ����İ༶
	classes[strlen(classes) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL,
			"INSERT INTO table1 (name, sex, age, class) VALUES ('%s', '%s', %s, '%s')",
			name, sex, age, classes); //
	//SQL = DELETE FROM table1 WHERE name = '������'
	printf("'%s'\n", SQL);
}

void updatename(char *SQL)
{
	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ�޸ĵ�����>:");
	write(STDOUT_FILENO, SQL, strlen(SQL)); //��仰�����printf�����ģ�����д�Ͳ���Ҫ\nҲ������Ļ�����
	char name[1024];
	memset(name, 0, sizeof(name));
	read(STDIN_FILENO, name, sizeof(name)); //�ȴ� �û�����Ҫ���������,�����û��������"������"
	name[strlen(name) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ�Ա���Ա�>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char sex[1024];
	memset(sex, 0, sizeof(sex));
	read(STDIN_FILENO, sex, sizeof(sex)); //�ȴ� �û�����Ҫ������Ա�,
	sex[strlen(sex) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ���������>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char age[1024];
	memset(age, 0, sizeof(age));
	read(STDIN_FILENO, age, sizeof(age)); //�ȴ� �û�����Ҫ���������
	age[strlen(age) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ�޸ĵİ༶>:");
	write(STDOUT_FILENO, SQL, strlen(SQL));
	char classes[1024];
	memset(classes, 0, sizeof(classes));
	read(STDIN_FILENO, classes, sizeof(classes)); //�ȴ� �û�����Ҫ����İ༶
	classes[strlen(classes) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'

	memset(SQL, 0, BUFSIZE);
	sprintf(SQL,
			"UPDATE table1 SET sex = '%s', age = %s, class = '%s' WHERE name = '%s'",
			sex, age, classes, name); //
	//SQL = DELETE FROM table1 WHERE name = '������'
	printf("'%s'\n", SQL);
}

void selectname(const char *SQL)
{
	/*
	char SQL[1024];
	memset(SQL, 0, BUFSIZE);
	sprintf(SQL, "%s", "������Ҫ��ѯ������>:");
	write(STDOUT_FILENO, SQL, strlen(SQL)); //��仰�����printf�����ģ�����д�Ͳ���Ҫ\nҲ������Ļ�����
	char name[1024];
	memset(name, 0, sizeof(name));
	read(STDIN_FILENO, name, sizeof(name)); //�ȴ� �û�����Ҫɾ��������,�����û��������"������"
	name[strlen(name) - 1] = 0; //���ַ������һλ\n�滻Ϊ0,���ŵ�������0���������ַ�'0'
	memset(SQL, 0, BUFSIZE);
	if (strlen(name) == 0) //�û�û���κ����룬ֻ�����˻س�������Ϊ0��
	{
		sprintf(SQL, "SELECT * FROM table2"); //
	} else
	{
		sprintf(SQL, "SELECT * FROM table2 where name = '%s'", name); //
	}

	*/
	if (mysql_query(connection, SQL) != 0)
	{
		printf("query error, %s\n", mysql_error(&mysql));
	}

	//����mysql_store_result�õ���ѯ���������ŵ�MYSQL_RES�ṹ����
	MYSQL_RES *result = mysql_store_result(connection);
	//Ҫ֪���������ݼ��ж����в������ɵ�ʹ�ø���SELECT���
	MYSQL_FIELD *field;
	int iFieldCount = 0;
	while (1)
	{
		field = mysql_fetch_field(result); //ѭ���õ����������ѭ�������е���󣬺�������NULL
		if (field == NULL)
			break;
		printf("%s\t", field->name);
		iFieldCount++;
	}
	printf("\n");

	//ѭ������ÿһ��
	MYSQL_ROW row;
	while (1)
	{
		row = mysql_fetch_row(result);
		if (row == NULL)
			break;
		int i = 0;
		for (; i < iFieldCount; i++)
		{
			printf("%s\t", (const char *) row[i]);
		}
		printf("\n");
	}
	mysql_free_result(result);
}

int main(int arg, char *args[])
{
	if (arg < 4)
		return -1;

	mysql_init(&mysql);		//�൱��SQL�ڲ���ʼ����һ��TCP��socket��ͬʱ��ʼ����SQL������ڴ��һЩ�ṹ

	//���ӵ�mysql server
	connection = mysql_real_connect(&mysql, args[1], args[2], args[3], args[4],
			0, 0, 0);
	if (connection == NULL)
	{
		printf("connect error, %s\n", mysql_error(&mysql));
		return -1;
	}

	if (mysql_query(connection, "SET NAMES utf8") != 0)		//�����ַ���ΪUTF8
	{
		printf("�����ַ�������, %s\n", mysql_error(&mysql));
	}

	char buf[BUFSIZE];
	memset(buf, 0, sizeof(buf));
	strcpy(buf, "��ѡ��\n1:����\n2:ɾ��\n3:�޸�\n4:��ѯ\n");
	write(STDOUT_FILENO, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	read(STDIN_FILENO, buf, sizeof(buf));
	if (strncmp(buf, "4", 1) == 0)		//�����û��������4
	{
		memset(buf, 0, sizeof(buf));
		strcpy(buf, "����������SELECt���");
		write(STDOUT_FILENO, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		read(STDIN_FILENO, buf, sizeof(buf));
		selectname(buf);
	} else
	{
		if (strncmp(buf, "1", 1) == 0)		//�����û��������1
		{
			insertname(buf);
		}

		if (strncmp(buf, "2", 1) == 0)		//�����û��������2
		{
			deletename(buf);
		}

		if (strncmp(buf, "3", 1) == 0)		//�����û��������3
		{
			updatename(buf);
		}
		mysql_query(connection, buf);
	}

	mysql_close(connection);		//�Ͽ���SQL server������

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}