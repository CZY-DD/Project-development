/*================================================================
*   Copyright (C) 2021 hqyj Ltd. All rights reserved.
*   
*   文件名称：cli.c
*   创 建 者：陈召杨
*   创建日期：2021年03月08日
*   描    述：菜鸟
*
================================================================*/
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#define Z 1
#define L 2
#define A 3
#define D 4
#define M 5
#define K 6
#define I 7
#define E 8
typedef struct {
	int type;
	int num;
	char name[20];
	char pswd[128];
	char data[256];
	char no[128];
	int  age;
	char sex[128];
	char salary[256];
	char tel[128];
	char permission[128];
}USER;
void do_add(int sfd,USER user);
void do_del(int sfd,USER user);
void do_mod(int sfd,USER user);
void do_inq_adm(int sfd,USER user);
void do_inq(int sfd,USER user);
void do_del_user(int sfd,USER user);

int main(int argc, char *argv[])
{
	//初始化
	struct sockaddr_in cin;
	char buf[128]={0};
	USER user;

	if(argc!=3){
		printf("参数不对\n");
		return -1;
	}
	int sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0){
		perror("socket error");
		return -1;
	}
	printf("创建套接字成功\n");
	cin.sin_family=AF_INET;
	cin.sin_port=htons(atoi(argv[2]));
	cin.sin_addr.s_addr=inet_addr(argv[1]);
	if(connect(sfd,(struct sockaddr *)&cin,sizeof(cin))<0){
		perror("connect error");
		return -1;
	}
	printf("连接中.....\n");
	homepage(sfd,&user);
	return 0;
}
int homepage(int sfd,USER user)
{
	int n;
	while(1){
		printf("******************(员工管理系统)***********\n");
		printf("*  1.register     2.login	    3.quit    *\n");
		printf("*******************************************\n");
		printf("请输入命令数字:");
		scanf("%d",&n);
		while(getchar()!='\n');
		if(n<=0 || n>3){
			printf("请重新输入命令数字");
			scanf("%d",&n);
		}
		switch(n){
			case 1:
				do_register(sfd,user);//注册
				break;
			case 2:
				do_login(sfd,user);
				break;
			case 3:
				close(sfd);
				return -1;
		}
	}
	return 0;
}
//注册
int do_register(int sfd,USER user)
{
	memset(&user,0,sizeof(USER));
	user.type=Z;
	printf("请输入注册的姓名:");
	scanf("%s",user.name);
	printf("请输入注册的密码:");
	scanf("%s",user.pswd);
	printf("请输入权限(admin/user):");
	scanf("%s",user.permission);

	if(write(sfd,&user,sizeof(USER))<0){
		perror("register write error");
		return -1;
	}
	if(read(sfd,&user,sizeof(USER))<0){
		perror("register read error");
		return -1;
	}
	printf("状态:%s\n",user.data);
	return 0;
}
//登录
int do_login(int sfd,USER user)
{
	int m;
	memset(&user,0,sizeof(USER));
	printf("请输入登录的名字:");
	scanf("%s",user.name);
	printf("请输入登录的密码:");
	scanf("%s",user.pswd);
	user.type=L;
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("%s\n",user.data);

	if(strncmp(user.permission,"admin",5)==0){
		while(1){
			printf("===================管理员用户==============\n");
			printf("=  1.添加  2.删除  3.修改  4.查询  5.退出 =\n");
			printf("===========================================\n");
			printf("请输入命令数字:");
			scanf("%d",&m);
			while(getchar()!='\n');
			if(m<=0 || m>5){
				printf("请重新输入命令数字.");
				scanf("%d",&m);
			}
			switch(m){
				case 1:
					do_add(sfd,user);
					break;
				case 2:
					do_del(sfd,user);
					break;
				case 3:
					do_mod(sfd,user);
					break;
				case 4:
					printf("请输入要查询的员工号:");
					scanf("%s",user.no);
					do_inq_adm(sfd,user);
					break;
				case 5:
					do_del_user(sfd,user);
					return 0;	
			}
		}
	}
	else if(strncmp(user.permission,"user",4)==0){
		while(1){
			printf("===========普通用户========\n");	
			printf("===   1.查询    2.退出  ===\n");
			printf("============================\n");
			printf("请输入对应的数字:");
			scanf("%d",&m);
			while(getchar()!='\n');
			if(m<=0 || m>3){
				printf("请重新输入命令数字.");
				scanf("%d",&m);

			}
		switch(m){
			case 1:
				printf("请输入要查询的员工号:");
				scanf("%s",user.no);
				do_inq(sfd,user);
				break;
			case 2:
				do_del_user(sfd,user);
				return 0;
			}
		}

	}
}
void do_add(int sfd,USER user)
{
	user.type=A;
	printf("请输入员工的工号:");
	scanf("%s",user.no);
	printf("请输入员工的名字:");
	scanf("%s",user.name); 
	printf("请输入员工的年龄:");
	scanf("%d",&user.age);
	printf("请输入员工的性别:");
	scanf("%s",user.sex);
	printf("请输入员工的工资:");
	scanf("%s",user.salary);
	printf("请输入员工的电话:");
	scanf("%s",user.tel);
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("%s\n",user.data); 
}
void do_del(int sfd,USER user)
{
	user.type=D;
	printf("请输入要删除的员工号:");
	scanf("%s",user.no);
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("%s\n",user.data);
}
//修改
void do_mod(int sfd,USER user)
{
	user.type=M;
	printf("请输入要修改的员工号:");
	scanf("%s",user.no);
	printf("================修改内容================\n");
	printf("== 1.名字 2.年龄 3.性别 4.工资 5.电话 ==\n");
	printf("========================================\n");
	printf("请输入要修改的序号:");
	scanf("%d",&user.num);
	while(getchar()!='\n');
	printf("请输入新的信息:");
	scanf("%s",user.data);

	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("%s\n",user.data);
}
//查询
void do_inq_adm(int sfd,USER user)
{ 
	user.type=K;
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("工号  名字  年龄  性别  工资  手机号\n");
	printf("%s\n",user.data);
}
void do_inq(int sfd,USER user)
{
	user.type=I;
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("工号  名字   手机号\n");
	printf("%s\n",user.data);
	
}
void do_del_user(int sfd,USER user)
{
	user.type=E;
	write(sfd,&user,sizeof(USER));
	read(sfd,&user,sizeof(USER));
	printf("%s\n",user.data);
}
