/* include serv01 */
#include <assert.h>
#include <leveldb/db.h>
#include "tools.h" 


extern "C" {
#include	"unp.h"
}

#define	MAXN	16384		/* max # bytes client can request */

#include	<sys/resource.h>

#ifndef	HAVE_GETRUSAGE_PROTO
int		getrusage(int, struct rusage *);
#endif

void
pr_cpu_time(void)
{
	double			user, sys;
	struct rusage	myusage, childusage;

	if (getrusage(RUSAGE_SELF, &myusage) < 0)
		err_sys("getrusage error");
	if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
		err_sys("getrusage error");

	user = (double) myusage.ru_utime.tv_sec +
					myusage.ru_utime.tv_usec/1000000.0;
	user += (double) childusage.ru_utime.tv_sec +
					 childusage.ru_utime.tv_usec/1000000.0;
	sys = (double) myusage.ru_stime.tv_sec +
				   myusage.ru_stime.tv_usec/1000000.0;
	sys += (double) childusage.ru_stime.tv_sec +
					childusage.ru_stime.tv_usec/1000000.0;

	printf("\nuser time = %g, sys time = %g\n", user, sys);
}

void
web_child(int sockfd,int index)
{
	int			ntowrite;
	ssize_t		read_len;
	char		read_buffer[MAXLINE], result[MAXN];

	printf("index = %d\n",index);
	void *ctx = zmq_init (1);
	assert (ctx);
	void *pb = zmq_socket (ctx, ZMQ_PULL);
	assert (pb);
	string head = makeToken(index);
	int rc = zmq_bind(pb, (std::string("ipc:///tmp/fanglf_to_server_")+head).c_str());
	//int rc = zmq_connect(sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);

	void *sb = zmq_socket (ctx, ZMQ_PUSH);
	assert (sb);
	rc = zmq_connect (sb, "ipc:///tmp/fanglf_zero_ipc");
	assert (rc == 0);

	std::string data,ret;
	for ( ; ; ) {
		char *ptr = read_buffer;
		read_len = 0;
		read_len = read(sockfd, ptr , MAXLINE);

		if (read_len == 0) {
			printf("reset by peer ,fd = %d erron=%d \n",sockfd,errno);
			rc = zmq_close (sb);
			assert (rc == 0);
			rc = zmq_close (pb);
			assert (rc == 0);
			rc = zmq_term (ctx);
			assert (rc == 0);
			return ;
		}

		//ntowrite = atol(line);
		//if ((ntowrite <= 0) || (ntowrite > MAXN))
		//	err_quit("client request for %d bytes", ntowrite);

		my_msg_send(sb,head+std::string(ptr,read_len));
		my_msg_recv(pb,ret);
		ret = ret.substr(8,ret.length()-8);
		Writen(sockfd, (void*)ret.data(), (ssize_t)ret.length());
	}



}

void dealdata(int index) {


}

void
sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		/* printf("child %d terminated\n", pid); */
	}
	return;
}

int
main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	//void				sig_chld(int), sig_int(int), web_child(int,int);
	void				sig_int(int);
	socklen_t			clilen, addrlen;
	struct sockaddr		*cliaddr;

	if (argc == 2)
		listenfd = Tcp_listen(NULL, argv[1], &addrlen);
	else if (argc == 3)
		listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
	else
		err_quit("usage: serv01 [ <host> ] <port#>");
	cliaddr =(sockaddr *) Malloc(addrlen);

	Signal(SIGCHLD, sig_chld);
	Signal(SIGINT, sig_int);

	int i=0;
	for ( ; ; ) {
		clilen = addrlen;
		if ( (connfd = accept(listenfd, cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}

		i++;
		if ( (childpid = Fork()) == 0) {	/* child process */
			Close(listenfd);	/* close listening socket */
			web_child(connfd,getpid());	/* process request */
			
			exit(0);
		}
		Close(connfd);			/* parent closes connected socket */
	}
}
/* end serv01 */

/* include sigint */
void
sig_int(int signo)
{
	void	pr_cpu_time(void);

	pr_cpu_time();
	exit(0);
}
/* end sigint */
