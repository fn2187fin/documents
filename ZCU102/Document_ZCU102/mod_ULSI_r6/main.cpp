#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "RxTx.hpp"
#include "main.hpp"

suseconds_t utime(){
	struct timeval t0;
	gettimeofday(&t0, NULL);
	return(t0.tv_usec);
}

#define WIDTH  256
#define HEIGHT 256
#define CHANNEL  3

#define CMD_NOP (0x0)
#define CMD_TOGLE_AFTER_BEFORE (0xA)
#define CMD_RESET_SUBFRAME_RNG (0xB)
#define CMD_CHANGE_ENGINE_MODE (0xC)

double elapsed_time(struct timeval before, struct timeval after){
  return (double)(after.tv_sec-before.tv_sec)+((double)(after.tv_usec-before.tv_usec))*1.0e-6;
}
void print_elapsed_time2(struct timeval before, struct timeval after){
  printf("elapsed_time: %g [sec]\n", elapsed_time(before, after));
}

int main(int argc,char**argv){
	int port=8888;
	char host[17]="";
	int cmd,arg1,arg2;
	static unsigned char header[]={
		0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5,0xA,0x5
	};
	int FlagAfterProc=1;
	static int header_size=sizeof(header);
	sprintf(host,"%s","192.168.1.100");
	if(argc>1){
		printf("command [port] [host]\n");
		printf("port :=%d\t(default)\n",port);
		port = atoi(argv[1]);
		printf("port :=%d\t(updated)\n",port);
	}
	if(argc>2){
		printf("host :=%s\t(default)\n",host);
		sprintf(host,"%s",argv[2]);
		printf("host :=%s\t(updated)\n",host);
	}
	inetRxTx rx(host, port);
	rx.Bind();

	int N=0, n=0, M=0, retry=0;
	int SubFrameNo=0, SubFrameBg=0, SubFrameEd=0, SubFrameNm=0;
	int engineMode=1;	// 1:reduced 2:normal 0:Pass
	unsigned char buf[CHANNEL*HEIGHT*WIDTH];
	unsigned char image[CHANNEL][HEIGHT][WIDTH];

	// Weight Ready
	printf("Reduced Weight Reading.. ");fflush(stdout);
	after_classify(PROC_INIT, image);
	printf("Ready\n");
	printf("Normal  Weight Reading.. ");fflush(stdout);
	before_classify(PROC_INIT, image);
	printf("Ready\n");
	int answer;
	struct timeval a,b,c,d;
	while(1){

		rx.Accept();

		gettimeofday(&a, NULL);

		memset(buf, 0, sizeof(buf));
		N=rx.recv_serv(buf,HEIGHT*WIDTH*CHANNEL);

		if(buf[0]==header[0] & buf[1]==header[1]){
			if(!memcmp(buf,header,header_size)){
				if(buf[header_size]==CMD_TOGLE_AFTER_BEFORE){
					// 0xA0x5*20Byte + Code
					FlagAfterProc=~FlagAfterProc;
					printf("CMD_TOGLE_AFTER_BEFORE\n");
				}else if(buf[header_size]==CMD_RESET_SUBFRAME_RNG){
					// 0xA0x5*20Byte + Code + Begin(0-14) + End(0-14)
					SubFrameNo=SubFrameBg=buf[header_size+1];
					SubFrameEd=buf[header_size+2];
					SubFrameNm=SubFrameEd-SubFrameBg+1;
					printf("CMD_RESET_SUBFRAME_RNG %d-%d/%d\n",
						SubFrameBg,SubFrameEd,SubFrameNm);
				}else if(buf[header_size]==CMD_CHANGE_ENGINE_MODE){
					// 0xA0x5*20Byte + Code + Mode
					int changeEngineMode=buf[header_size+1];
					printf("CMD_CHANGE_ENGINE_MODE %d\n", engineMode);
					if(changeEngineMode>0)
						engineMode=changeEngineMode;
				}
				memset(buf,0,6);
				buf[0]=(unsigned char)buf[header_size];
				M=rx.send_serv(buf,6);	// return recieved command code
				rx.disconnect();
				continue;
			}
		}

		gettimeofday(&c, NULL);

		memcpy(image,buf,CHANNEL*HEIGHT*WIDTH);
		if(engineMode==1)
			answer = after_classify(PROC_RECOG,image);
		else if(engineMode==2)
			answer = before_classify(PROC_RECOG,image);
		else
			answer = 5;

		gettimeofday(&d, NULL);
		printf("Sub-Frame:%4d:\tanswer=%d\n",SubFrameNo,answer);
		print_elapsed_time2(c, d);	// time for classify

		//printf("%d read\n",N);
		memset(buf, 0, sizeof(buf));
		buf[0]=(unsigned char)SubFrameNo;
		buf[1]=0x00;	//clear a byte for car
		buf[2]=0x00;	//clear a byte for pedestrian
		buf[3]=0x00;	//clear a byte for sign
		buf[4]=0x00;	//clear a byte for light
		buf[5]=0x00;	//clear a byte for nothing
		// change answer from 0-4 to 1-5, then answer==1 is car
		// answer 0 means ignore
		buf[answer+1] = answer+1;
		M=rx.send_serv(buf,6);

		//printf("N=%d read M=%d send\n",N,M);
		if(++SubFrameNo>SubFrameEd) SubFrameNo=SubFrameBg;
		rx.disconnect();

		gettimeofday(&b, NULL);
		printf("elapsed_time: %g [sec] networking\n", elapsed_time(a, b)-elapsed_time(c, d));
	}
	return 0;
}

