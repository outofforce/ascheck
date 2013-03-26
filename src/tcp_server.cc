#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <signal.h>
#include <assert.h>
#include <map>
#include <leveldb/db.h>
#include "tools.h" 


#define MAXLINE 1024 
#define OPEN_MAX 100
#define LISTENQ 20
#define MAX_EVENT 20
#define SERV_PORT 5023 
#define INFTIM 1000
#define YL_READ 0 
#define YL_WRITE 1 
#define MAX_MQ_LEN 512 
#define MAX_MQ_MSG_COUNT 10 
using namespace std;

struct task{
	int fd;            
	struct task *next; 
	struct user_data *data ;

};

struct user_data{
	int fd;
	unsigned int n_size;
	char line[MAXLINE];
};

void * readtask(void *args);
void * writetask(void *args);
struct epoll_event ev,events[20];
int epfd;
pthread_mutex_t mutex;
pthread_cond_t cond1;

struct task *readhead=NULL,*readtail=NULL,*writehead=NULL;
void *ctx;
void setnonblocking(int sock)
{
	int flags = 1;
	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof( flags ) );
	int opts;
	opts=fcntl(sock, F_GETFL);
	if(opts<0)
	{
		perror("fcntl(sock,GETFL)");
		exit(1);
	}
	opts = opts | O_NONBLOCK;
	if(fcntl(sock, F_SETFL, opts)<0)
	{
		perror("fcntl(sock,SETFL,opts)");
		exit(1);
	}
}

void ctrl_c(int sig)
{
	printf("CTRL+C, ...\n");
	//b_ctrl_c = true;
	exit(0);
}

int 
main(int argc, char **argv)
{
	signal(SIGINT, ctrl_c);
	int i,  listenfd, connfd, sockfd,nfds;
	int c;

/*
	while ( (c = Getopt(argc, argv, "n:l:")) != -1) {
		switch (c) {
			case 'n':
				maxmq= atol(optarg);
				break;
			case 'l':
				yl_logfile = optarg;
				break;
		}
	}
	*/

	pthread_t tid1,tid2;
	struct task *new_task = NULL;
	struct user_data *rdata = NULL;
	socklen_t clilen;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond1, NULL);

	ctx = zmq_ctx_new();
	assert (ctx);
	pthread_create(&tid1, NULL, readtask, NULL);
	//pthread_create(&tid2, NULL, readtask, NULL);
	pthread_create(&tid2, NULL, writetask, NULL);


	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(1);
	}

	setnonblocking(listenfd);

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	const char *local_addr = "127.0.0.1";
	inet_aton(local_addr, &(serveraddr.sin_addr)); //htons(SERV_PORT);
	serveraddr.sin_port=htons(3000);

	if (bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
		perror("bind error");
		exit(1);
	}

	if (listen(listenfd, LISTENQ) == -1) {
		perror("listen error");
		exit(1);
	}

	epfd = epoll_create(256);
	clilen = sizeof(struct sockaddr_in);
	ev.events = EPOLLIN ;
	ev.data.fd = listenfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
		fprintf(stderr, "epoll set insertion error: fd=%d\n", listenfd);
		exit(1);
	} else
		printf("socket epoll \n");

	for ( ; ; ) {

		nfds=epoll_wait(epfd, events, MAX_EVENT, -1); 

		for(i=0; i < nfds; ++i)
		{
			if(events[i].data.fd==listenfd)
			{
				connfd = accept(listenfd,(struct sockaddr *)&clientaddr, &clilen);
				if(connfd<0){
					perror("connfd<0");
					continue;
				}
				printf("client_addr=%s :port=%d: fd=%d\n", 
						inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), connfd);
				setnonblocking(connfd);

				ev.data.fd=connfd;

				ev.events=EPOLLIN | EPOLLET;

				if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
					fprintf(stderr, " socket (%d)  epoll %s\n",
							connfd, strerror(errno));
					close(connfd);
					continue;
				}

			} else if(events[i].events & EPOLLIN) {

				if ( (sockfd = events[i].data.fd) < 0) continue;

				new_task = new task();
				new_task->fd =sockfd;
				new_task->next = NULL;
				//new_task->type=YL_READ;
				new_task->data=NULL;

				// create a read task 
				pthread_mutex_lock(&mutex);

				if(readhead == NULL)
				{
					readhead = new_task;
					readtail = new_task;
				}
				else
				{
					readtail->next = new_task;
					readtail = new_task;
				}
				pthread_cond_broadcast(&cond1);
				pthread_mutex_unlock(&mutex);

			} else if(events[i].events & EPOLLOUT) {
	
				rdata=(struct user_data *)events[i].data.ptr;
				sockfd = rdata->fd;
				int nwrite;
				int n = rdata->n_size;  
				//printf("in EPOLLOUT write to fd \n");
				while (n > 0) {  
					nwrite = write(sockfd, rdata->line + rdata->n_size - n, n);  
					//printf("in EPOLLOUT write  1  \n");
					if (nwrite < n) {
						if (nwrite == -1 && errno != EAGAIN) {
							printf("BBBBBBAAAAAADDDDDDD write error\n");
						}
						break;
					}
					n -= nwrite;
				} 

				delete rdata;
				ev.data.fd=sockfd;
				ev.events=  EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
			}
		}
	}
}

void * readtask(void *args)
{
	int fd=-1;
	int n;
	int type;
	struct user_data *data = NULL;
	long read_len=0; 
	char read_buffer[1024]; 
	void *to_ipc_server = zmq_socket (ctx, ZMQ_PUSH);
	assert (to_ipc_server);
	int rc = zmq_connect(to_ipc_server, "ipc:///tmp/fanglf_zero_ipc");
	//int rc = zmq_bind (sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);

/*
	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = false;
	leveldb::Status status = leveldb::DB::Open(options, "/tmp/fanglf_leveldb_ipc", &db);
	if(!status.ok()) {
		cerr<<status.ToString()<<endl;
		return 0;
	}
	std::string value="1";
	*/


	printf("readtask working\n");
	while(1){
		pthread_mutex_lock(&mutex);

		while(readhead == NULL)
			pthread_cond_wait(&cond1, &mutex); 

		fd = readhead->fd;
		data = readhead->data;

		struct task *tmp = readhead;
		readhead = readhead->next;
		delete tmp;
		pthread_mutex_unlock(&mutex);

		{
			char *ptr = read_buffer;
			read_len = 0;

			while ((n= read(fd, ptr + read_len, MAXLINE)) > 0) { 
				read_len += n;
				assert(read_len<=MAXLINE);
			}

			if (read_len == 0 || (n = -1 && errno != EAGAIN)) {
				//printf("close fd = %d n=%d len=%d erron=%d \n",fd,n,read_len,errno);
				if ((n == 0) || (errno == ECONNRESET))  {
					printf(" fd(%d) reset by peer\n",fd);
					close(fd);
				}

				continue;
			} 


			read_buffer[read_len]='\0';
			
			my_msg_send(to_ipc_server,makeToken(fd)+read_buffer);
			/*
			data = new user_data();
			data->fd=fd;
			bzero(data->line,MAXLINE);
			memcpy(data->line,read_buffer,read_len);
			data->n_size = read_len;
			epoll_event myevn;
			myevn.data.fd=data->fd;
			myevn.events=EPOLLOUT | EPOLLET;
			myevn.data.ptr=data;
			epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &myevn);
			*/

		}
	}
}

void * writetask(void *args)
{
	int fd=-1;
	struct user_data *data = NULL;
	void *from_ipc_server = zmq_socket (ctx, ZMQ_PULL);
	assert (from_ipc_server);
	int rc = zmq_bind(from_ipc_server, "ipc:///tmp/fanglf_to_server");
	//int rc = zmq_bind (sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);
	string recive_data = ""; 
	while (true) {
		my_msg_recv(from_ipc_server,recive_data);
		fd = getToken(recive_data);

		data = new user_data();
		data->fd=fd;
		bzero(data->line,MAXLINE);
		memcpy(data->line,recive_data.data()+8,recive_data.length()-8);
		data->n_size = recive_data.length()-8;
		epoll_event myevn;
		myevn.data.fd=data->fd;
		myevn.events=EPOLLOUT | EPOLLET;
		myevn.data.ptr=data;
		epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &myevn);
	}
}
