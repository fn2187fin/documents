#ifndef _RXTX_HPP_
#define _RXTX_HPP_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
class inetRxTx {
	private:
		struct sockaddr_in addr_serv;
		struct sockaddr_in addr_conn;
		int sock0;
		struct sockaddr_in client;
		int len;
		int sock_serv;
		int sock_conn;
	public:
	inetRxTx(const char *ipaddrname, unsigned int port){
		sock0 = socket(AF_INET, SOCK_STREAM, 0);
	// socket for Server
		addr_serv.sin_family		= AF_INET;
		addr_serv.sin_port			= htons(port);
		addr_serv.sin_addr.s_addr	= INADDR_ANY;
	// socket for Client
		addr_conn					= addr_serv;
		addr_conn.sin_addr.s_addr	= inet_addr(ipaddrname);
	}
	~inetRxTx(){close(sock_serv);}
	void Bind(){
	// bind and wait
		if(bind(sock0, (struct sockaddr *)&addr_serv, sizeof(addr_serv))){printf("bind error\n"); return;}
		if(listen(sock0, 5)){printf("listen error\n");return;}
	}
	int Accept(){
		int N=0, n=0, m=0;
		len = sizeof(client);
		sock_serv = accept(sock0, (struct sockaddr *)&client, (socklen_t*)&len);
		return(sock_serv);
	}
	void disconnect(){close(sock_serv);}
	int recv_serv(unsigned char *buf, unsigned int len){
		int offset=0, done;
		do{
			done=read(sock_serv, buf+offset, len);
			if(!done)break;
			offset+=done;
		} while(len != offset);
		return(offset);
	}
	int send_serv(unsigned char *buf, int len){
		int offset=0, done;
		do{
			done=write(sock_serv, buf+offset , len);
			if(!done)break;
			offset+=done;
		} while(len != offset);
		return(offset);
	}
	int Connect(){
		int ret;
		sock_conn = socket(AF_INET, SOCK_STREAM, 0);
		while((ret=connect(sock_conn, (struct sockaddr *)&addr_conn, sizeof(addr_conn)))!=0);
		printf("connect\n");
		return(ret);
	}
	int recv_conn(unsigned char *buf, int len){
		int offset=0, done;
		do{
			done=read(sock_conn, buf+offset, sizeof(buf));
			if(!done)break;
			offset+=done;
		} while(len != offset);
		return(offset);
	}
	int send_conn(unsigned char *buf, int len){
		int offset=0, done;
		do{
			done=write(sock_conn, buf+offset , len);
			if(!done)break;
			offset+=done;
		} while(len != offset);
		return(offset);
	}
	void eoc(){close(sock_conn);}
};

#endif
