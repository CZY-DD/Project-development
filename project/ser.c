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
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <errno.h>

#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100
#define Z 1
#define L 2
#define A 3
#define D 4
#define M 5
#define K 6
#define I 7
#define E 8
typedef struct{
	int type;
	int num;
	char name[20];
	char pswd[128];
	char data[256];
	char no[128];
	int age;
	char sex[128];
	char salary[256];
	char tel[128];
	char permission[128];
}USER;

//IO多路复用epoll
void do_epoll(int listenfd,USER user);
//事件处理函数
void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,USER user);
//处理接收到的连接
void handle_accpet(int epollfd,int listenfd);
//读处理
void do_read(int epollfd,int fd,USER user);
//写处理
void do_write(int epollfd,int fd,USER ser);
//添加事件
void add_event(int epollfd,int fd,int state);
//修改事件
void modify_event(int epollfd,int fd,int state);
//删除事件
void delete_event(int epollfd,int fd,int state);

void do_log_in(int fd,USER user);
void do_add(int fd,USER user);
void do_del(int fd,USER user);
void do_mod(int fd,USER user);
void do_online_user(int fd,USER user);
void do_inq_user(int fd,USER user);
void do_del_user(int fd,USER user);

sqlite3 *db=NULL;
char sql[512]="";
USER user;
int main(int argc, char *argv[])
{
	int sfd;
	struct sockaddr_in sin;
	char *errmsg=NULL;
	//创建数据库
	if(sqlite3_open("./my.db",&db)!=0){
		printf("open sqlite3 failed\n");
		return -1;
	}
	printf("数据库打开或创建成功\n");
	bzero(sql,sizeof(sql));
	sprintf(sql,"create table if not exists user(name char primary key,pswd char,permission char)");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		fprintf(stderr,"sqlite3_exec1:%s\n",errmsg);
		goto END;
	}
	printf("创建user表格成功\n");
	
	bzero(sql,sizeof(sql));
	sprintf(sql,"create table if not exists info(no char primary key,name char,age int,sex char,salary char,tel char)");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		fprintf(stderr,"sqlite3_exec1:%s\n",errmsg);
		goto END;
	}
	printf("创建info表成功\n");
	bzero(sql,sizeof(sql));
	sprintf(sql,"create table if not exists online(name char primary key)");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
		fprintf(stderr,"online create error:%s\n",errmsg);
		goto END;
	}
	printf("创建online表成功\n");
	//创建socket
	sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0){
		perror("socket error");
		return -1;
	}
	printf("套接字创建成功\n");
	char buf[128]={0};
	int reuse=1;
	setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
	sin.sin_family=AF_INET;
	sin.sin_port=htons(1234);
	sin.sin_addr.s_addr=inet_addr("0");
	if(bind(sfd,(struct sockaddr *)&sin,sizeof(sin))<0)
	{
		perror("bind error");
		return -1;
	}
	printf("绑定成功\n");
	if(listen(sfd,5)<0){
		perror("listen error");
		return -1;
	}
	do_epoll(sfd,user);

END:
	if(sqlite3_close(db)!=0)
	{
		fprintf(stderr,"数据库关闭失败\n");
			return -1;
	}
	return 0;
}
//多路复用
void do_epoll(int listenfd,USER user)
{
	int epollfd; 
	struct epoll_event events[100];
	int ret;
	//创建一个描述符
	epollfd = epoll_create(1000);
	//添加监听描述符事件
	add_event(epollfd,listenfd,EPOLLIN);
	for ( ; ; )
	{
		//获取已经准备好的描述符事件
		ret = epoll_wait(epollfd,events,100,-1);
		handle_events(epollfd,events,ret,listenfd,user);
	}
	close(epollfd);
}
//事件处理函数
void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,USER user)
{
	int i;
	int fd;
	//进行选好遍历
	for (i = 0;i < num;i++)
	{
		fd = events[i].data.fd;
		//根据描述符的类型和事件类型进行处理
		if ((fd == listenfd) &&(events[i].events & EPOLLIN))
			handle_accpet(epollfd,listenfd);
		else if (events[i].events & EPOLLIN)
			do_read(epollfd,fd,user);
		else if (events[i].events & EPOLLOUT)
			do_write(epollfd,fd,user);
	}
}
//处理接收到的连接
void handle_accpet(int epollfd,int listenfd)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t len=sizeof(len);
	clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&len);
	if (clifd == -1)
		perror("accpet error");
	else
	{
		char ip[20]="";
		inet_ntop(AF_INET,&cliaddr.sin_addr,ip,20);
		printf("[%s:%d]->连接成功\n",ip,ntohs(cliaddr.sin_port));
		//添加一个客户描述符和事件
		add_event(epollfd,clifd,EPOLLIN);
	}
}
void do_read(int epollfd,int fd,USER user)
{
	int nread;
	nread = read(fd,&user,sizeof(user));
	if (nread == -1)
	{
		perror("read error");
		close(fd);
		delete_event(epollfd,fd,EPOLLIN);
	}
	else if (nread == 0)
	{
		fprintf(stderr,"client close\n");
		close(fd);
		delete_event(epollfd,fd,EPOLLIN);
	}
	else
	{
	//	printf("read message is:%d\n",user.type);
		switch(user.type){
			case Z:  //注册
				do_register(fd,user);
				break;
			case L:  //登录
				do_log_in(fd,user);
				break;
			case A:  //添加
				do_add(fd,user);
				break;
			case D:  //删除
				do_del(fd,user);
				break;
			case M:  //修改
				do_mod(fd,user);
				break;
			case K:  //查询
				do_look_up(fd,user);
				break;
			case I:   //普通用户查看
				do_inq_user(fd,user);
				break;
			case E:  //删除在线用户表
				do_del_user(fd,user);
				break;
		}

		//修改描述符对应的事件，由读改为写
	//	modify_event(epollfd,fd,EPOLLOUT);
	}
} 
void do_write(int epollfd,int fd,USER user)
{
	int nwrite;
	memset(&user,0,sizeof(user));
	strcpy(user.data,"register ok");
	nwrite = write(fd,&user,sizeof(user));
	if(nwrite == -1)
	{
		perror("write error:");
		close(fd);
		delete_event(epollfd,fd,EPOLLOUT);
	}
	else
	modify_event(epollfd,fd,EPOLLIN);
}
void add_event(int epollfd,int fd,int state)
{
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
} 
void delete_event(int epollfd,int fd,int state)
{
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
} 
void modify_event(int epollfd,int fd,int state)
{
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}
int do_register(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from user where name ='%s'",user.name);
	printf("%s\n",sql);
    sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp>0){
		printf("用户已存在\n");
		strcpy(user.data,"user exit");
		write(fd,&user,sizeof(user));	
	}
	else{
		printf("无用户数据\n");
		sprintf(sql,"insert into user values('%s','%s','%s')",user.name,user.pswd,user.permission);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
		{ 
			printf("插入表格失败\n");
			return -1;
		}
		printf("注册成功\n");
		strcpy(user.data,"OK");
		write(fd,&user,sizeof(user));
	}
	return 0;
}
//登录
void do_log_in(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from user where name ='%s' and pswd= '%s'",user.name,user.pswd);
	printf("%s\n",sql);
	sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp==0)
	{
		printf("登录失败\n");
		strcpy(user.data,"user_name/pswd failed");
		write(fd,&user,sizeof(user));
	}
	else if(rowp==1){
		bzero(sql,sizeof(sql));
		sprintf(sql,"insert into online values('%s')",user.name);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0){
			printf("insert online error\n");
			strcpy(user.data,"exit");
			write(fd,&user,sizeof(USER));
		}
		else{
			int i;
			for(i=0;i<(rowp+1)*columnp;i++)
			{
				if(strcmp(resultp[i],user.name)==0){
				printf("%s\n",resultp[i+2]);
				sprintf(user.permission,"%s",resultp[i+2]);
				}
			}
			printf("登录成功\n");
			strcpy(user.data,"login succeed");
			write(fd,&user,sizeof(user));
		}
	} 
}
void do_add(int fd,USER user)
{
	char *errmsg;
	bzero(sql,sizeof(sql));
	sprintf(sql,"insert into info values('%s','%s','%d','%s','%s','%s')",user.no,user.name,user.age,user.sex,
			user.salary,user.tel);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
	{
		strcpy(user.data,"add failed");
		write(fd,&user,sizeof(user));
	}
	else{
	
		printf("添加成功\n");
		strcpy(user.data,"add succeed");
		write(fd,&user,sizeof(user));
	}
	
}
void do_del(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from info where no='%s'",user.no);
	printf("%s\n",sql);
	sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp==0){
		printf("delete failed\n");
		strcpy(user.data,"del failed");
		write(fd,&user,sizeof(user));
	}
	else{
		sprintf(sql,"delete from info where no='%s'",user.no);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
		{
			printf("delete failed\n");	
		}
		else{
		printf("delete succeed\n");
		strcpy(user.data,"dalete succeed");
		write(fd,&user,sizeof(user));
		}
	}
}
int do_look_up(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	int i;
	
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from info where no='%s'",user.no);
	sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp==0)
	{
		printf("select error\n");
		strcpy(user.data,"select failed");
		write(fd,&user,sizeof(user));
		return -1;
	}
	else{
		printf("select info succeed\n");
	//	printf("rowp:%d columnp:%d\n",rowp,columnp);	
		for(i=0;i<(rowp+1)*columnp;i++){
			if(strcmp(resultp[i],user.no)==0){
				sprintf(user.data,"%s  %s   %d   %s  %s  %s",resultp[i],resultp[i+1],atoi(resultp[i+2]),resultp[i+3],
						resultp[i+4],resultp[i+5]);
			}
		}
		printf("data:%s\n",user.data); 
		write(fd,&user,sizeof(user));
	}
	return 0;
}
void do_mod(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from info where no='%s'",user.no);
	sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp==0){
		printf("selec error\n");
		strcpy(user.data,"no failed");
		write(fd,&user,sizeof(user));
	}
	else{
		printf("%s %s\n",user.data,user.no);
		bzero(sql,sizeof(sql));
		switch(user.num){
			case 1: //名字
				sprintf(sql,"update info set name ='%s' where no = '%s'",user.data,user.no);
				printf("%s\n",sql);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
				{
					printf("updata info error\n");
					strcpy(user.data,"mod name error");
					write(fd,&user,sizeof(USER));
				}
				else{
					printf("updata info succeed");
					strcpy(user.data,"mod name succeed");
					write(fd,&user,sizeof(USER));
				}
				break;
			case 2: //年龄
				sprintf(sql,"update info set age ='%s' where no = '%s'",user.data,user.no);
				printf("%s\n",sql);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
				{
					printf("updata info error\n");
					strcpy(user.data,"mod age error");
					write(fd,&user,sizeof(USER));
				}
				else{
					printf("updata info succeed");
					strcpy(user.data,"mod age succeed");
					write(fd,&user,sizeof(USER));
				}
				break;
			case 3: //性别
				sprintf(sql,"update info set sex ='%s' where no = '%s'",user.data,user.no);
				printf("%s\n",sql);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
				{
					printf("updata info error\n");
					strcpy(user.data,"mod sex error");
					write(fd,&user,sizeof(USER));
				}
				else{
					printf("updata info succeed");
					strcpy(user.data,"mod sex succeed");
					write(fd,&user,sizeof(USER));
				}
				break;
			case 4: //工资
				sprintf(sql,"update info set salary ='%s' where no = '%s'",user.data,user.no);
				printf("%s\n",sql);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
				{
					printf("updata info error\n");
					strcpy(user.data,"mod salary error");
					write(fd,&user,sizeof(USER));
				}
				else{
					printf("updata info succeed");
					strcpy(user.data,"mod salary succeed");
					write(fd,&user,sizeof(USER));
				}
				break;
			case 5: //电话
				sprintf(sql,"update info set tel ='%s' where no = '%s'",user.data,user.no);
				printf("%s\n",sql);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
				{
					printf("updata info error\n");
					strcpy(user.data,"mod tel error");
					write(fd,&user,sizeof(USER));
				}
				else{
					printf("updata info succeed");
					strcpy(user.data,"mod tel succeed");
					write(fd,&user,sizeof(USER));
				}
				break;

		}
	}
}
void do_inq_user(int fd,USER user)
{
	char *errmsg;
	char **resultp;
	int rowp,columnp;
	int i;
	bzero(sql,sizeof(sql));
	sprintf(sql,"select * from info where no='%s'",user.no);
	printf("%s\n",sql);
	sqlite3_get_table(db,sql,&resultp,&rowp,&columnp,&errmsg);
	if(rowp == 0){
		printf("user select failed\n");
		strcpy(user.data,"user look error");
		write(fd,&user,sizeof(user));
	}
	else{
		for(i=0;i<(rowp+1)*columnp;i++){
			if(strcmp(resultp[i],user.no)==0){
				sprintf(user.data,"%s  %s   %s",resultp[i],resultp[i+1],resultp[i+5]);
			}
		}
		printf("user select succeed\n");
		write(fd,&user,sizeof(user));
	}
}
void do_del_user(int fd,USER user)
{
	char *errmsg;
	bzero(sql,sizeof(sql));
	sprintf(sql,"delete from online where name=\"%s\"",user.name);
	printf("%s\n",sql);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("删除表失败\n");
	}
	else{
		strcpy(user.data,"exit ok");
		write(fd,&user,sizeof(USER));
		printf("删除%s 成功\n",user.name);
	}
}
