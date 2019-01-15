#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define BUFSIZE 1024
#define THRESHOLD 1
#define DEBUG
//#define USE_BUCKET
#define Y 256
#define TIME
//#define USE_FIX

union binary_float{
  float f;
  unsigned int a;
};

struct convolution{
  union binary_float ***coef;
  union binary_float bias;
};

struct net_param{
  int num_conv;
  int x, y, z;
  struct convolution *conv;
};

struct mnist_data{
  unsigned char data[1][28][28];
  int label;
};

struct mnist_dataset{
  struct mnist_data *set;
  int number;
};

struct cifar_data{
  unsigned char data[3][32][32];
  int label;
};

struct cifar_dataset{
  struct cifar_data *set;
  int number;
};

struct layer{
  int x, y, z, pad;
  union binary_float *value;
  int n;
  // (void*) is (int*) or (union binary_float*)
  // number of element is n=(x*(y+2*pad)*(z+2*pad)+1)
  // layer[i, j, k] is accessed as value[(i*y+j+2*pad)*z+k+2*pad]
  // value[((x-1)*(y+2*pad)+(y-1)+2*pad)*(z+2*pad)+(z-1)+2*pad+1] = 1.0f
};

struct matrix{
  union binary_float *value;
  int n, m;
  // nxm matirx: 
  // matrix[i,j] is accessed as value[i*m+j]
  int x, y, z;
  // x, y, and z are the dimension of convolusion.
  // m = x*y*z+1
  // conv[i, j, k, l] is accessed as value[i*m+(j*y+k)*z+l]
  // i:[0,n), j:[0,x), k:[0,y), l:[0,z)
};

int global_calc_number;

/* bitmap.c */

#define FILEHEADERSIZE 14					//ファイルヘッダのサイズ
#define INFOHEADERSIZE 40					//情報ヘッダのサイズ
#define HEADERSIZE (FILEHEADERSIZE+INFOHEADERSIZE)

typedef struct{
	unsigned char b;
	unsigned char g;
	unsigned char r;
}Rgb;

typedef struct{
        int height;
	int width;
	Rgb *data;
}Image;

//取得に成功すればポインタを失敗すればNULLを返す
Image *Read_Bmp(char *filename);

//書き込みに成功すれば0を失敗すれば1を返す
int Write_Bmp(char *filename, Image *img);

//Imageを作成し、RGB情報もwidth*height分だけ動的に取得する。
//成功すればポインタを、失敗すればNULLを返す
Image *Create_Image(int width, int height);
//Imageを解法する
void Free_Image(Image *img);

//filenameのBitmapファイルを読み込み、高さと幅、RGB情報をimg構造体に入れる
Image *Read_Bmp(char *filename)
{
	int i, j;
	int real_width;					//データ上の1行分のバイト数
	int width, height;			//画像の横と縦のピクセル数
	unsigned int color;			//何bitのBitmapファイルであるか
	FILE *fp;
	char header_buf[HEADERSIZE];	//ヘッダ情報を取り込む
	unsigned char *bmp_line_data;  //画像データ1行分
	Image *img;

	if((fp = fopen(filename, "rb")) == NULL){
		fprintf(stderr, "Error: %s could not read.", filename);
		return NULL;
	}

	fread(header_buf, sizeof(unsigned char), HEADERSIZE, fp); //ヘッダ部分全てを取り込む

	//最初の2バイトがBM(Bitmapファイルの印)であるか
	if(strncmp(header_buf, "BM", 2)){
		fprintf(stderr, "Error: %s is not Bitmap file.", filename);
		return NULL;
	}

	memcpy(&width, header_buf + 18, sizeof(width)); //画像の見た目上の幅を取得
	memcpy(&height, header_buf + 22, sizeof(height)); //画像の高さを取得
	memcpy(&color, header_buf + 28, sizeof(unsigned int)); //何bitのBitmapであるかを取得

	//24bitで無ければ終了
	if(color != 24){
		fprintf(stderr, "Error: %s is not 24bit color image", filename);
		return NULL;
	}

	//RGB情報は画像の1行分が4byteの倍数で無ければならないためそれに合わせている
	real_width = width*3 + width%4;

	//画像の1行分のRGB情報を取ってくるためのバッファを動的に取得
	if((bmp_line_data = (unsigned char *)malloc(sizeof(unsigned char)*real_width)) == NULL){
		fprintf(stderr, "Error: Allocation error.\n");
		return NULL;
	}

	//RGB情報を取り込むためのバッファを動的に取得
	if((img = Create_Image(width, height)) == NULL){
		free(bmp_line_data);
		fclose(fp);
		return NULL;
	}

	//BitmapファイルのRGB情報は左下から右へ、下から上に並んでいる
	for(i=0; i<height; i++){
		fread(bmp_line_data, 1, real_width, fp);
		for(j=0; j<width; j++){
			img->data[(height-i-1)*width + j].b = bmp_line_data[j*3];
			img->data[(height-i-1)*width + j].g = bmp_line_data[j*3 + 1];
			img->data[(height-i-1)*width + j].r = bmp_line_data[j*3 + 2];
		}
	}

	free(bmp_line_data);

	fclose(fp);

	return img;
}

int Write_Bmp(char *filename, Image *img)
{
	int i, j;
	FILE *fp;
	int real_width;
	unsigned char *bmp_line_data; //画像1行分のRGB情報を格納する
	unsigned char header_buf[HEADERSIZE]; //ヘッダを格納する
	unsigned int file_size;
	unsigned int offset_to_data;
	unsigned long info_header_size;
	unsigned int planes;
	unsigned int color;
	unsigned long compress;
	unsigned long data_size;
	long xppm;
	long yppm;

	if((fp = fopen(filename, "wb")) == NULL){
		fprintf(stderr, "Error: %s could not open.", filename);
		return 1;
	}

	real_width = img->width*3 + img->width%4;

	//ここからヘッダ作成
	file_size = img->height * real_width + HEADERSIZE;
	offset_to_data = HEADERSIZE;
	info_header_size = INFOHEADERSIZE;
	planes = 1;
	color = 24;
	compress = 0;
	data_size = img->height * real_width;
	xppm = 1;
	yppm = 1;
	
	header_buf[0] = 'B';
	header_buf[1] = 'M';
	memcpy(header_buf + 2, &file_size, sizeof(file_size));
	header_buf[6] = 0;
	header_buf[7] = 0;
	header_buf[8] = 0;
	header_buf[9] = 0;
	memcpy(header_buf + 10, &offset_to_data, sizeof(file_size));
	header_buf[11] = 0;
	header_buf[12] = 0;
	header_buf[13] = 0;

	memcpy(header_buf + 14, &info_header_size, sizeof(info_header_size));
	header_buf[15] = 0;
	header_buf[16] = 0;
	header_buf[17] = 0;
	memcpy(header_buf + 18, &img->width, sizeof(img->width));
	memcpy(header_buf + 22, &img->height, sizeof(img->height));
	memcpy(header_buf + 26, &planes, sizeof(planes));
	memcpy(header_buf + 28, &color, sizeof(color));
	memcpy(header_buf + 30, &compress, sizeof(compress));
	memcpy(header_buf + 34, &data_size, sizeof(data_size));
	memcpy(header_buf + 38, &xppm, sizeof(xppm));
	memcpy(header_buf + 42, &yppm, sizeof(yppm));
	header_buf[46] = 0;
	header_buf[47] = 0;
	header_buf[48] = 0;
	header_buf[49] = 0;
	header_buf[50] = 0;
	header_buf[51] = 0;
	header_buf[52] = 0;
	header_buf[53] = 0;

	//ヘッダの書き込み
	fwrite(header_buf, sizeof(unsigned char), HEADERSIZE, fp);
	
	if((bmp_line_data = (unsigned char *)malloc(sizeof(unsigned char)*real_width)) == NULL){
		fprintf(stderr, "Error: Allocation error.\n");
		fclose(fp);
		return 1;
	}

	//RGB情報の書き込み
	for(i=0; i<img->height; i++){
		for(j=0; j<img->width; j++){
			bmp_line_data[j*3]			=	img->data[(img->height - i - 1)*img->width + j].b;
			bmp_line_data[j*3 + 1]	=	img->data[(img->height - i - 1)*img->width + j].g;
			bmp_line_data[j*3 + 2]			=	img->data[(img->height - i - 1)*img->width + j].r;
		}
		//RGB情報を4バイトの倍数に合わせている
		for(j=img->width*3; j<real_width; j++){
			bmp_line_data[j] = 0;
		}
		fwrite(bmp_line_data, sizeof(unsigned char), real_width, fp);
	}

	free(bmp_line_data);

	fclose(fp);

	return 0;
}

Image *Create_Image(int width, int height)
{
	Image *img;

	if((img = (Image *)malloc(sizeof(Image))) == NULL){
		fprintf(stderr, "Allocation error\n");
		return NULL;
	}

	if((img->data = (Rgb*)malloc(sizeof(Rgb)*width*height)) == NULL){
		fprintf(stderr, "Allocation error\n");
		free(img);
		return NULL;
	}

	img->width = width;
	img->height = height;

	return img;
}

//動的に取得したRGB情報の開放
void Free_Image(Image *img)
{
	free(img->data);
	free(img);
}


void abort_stop(char *message){
  fprintf(stderr, "%s.\n", message);
  exit(1);
}

unsigned int endian(unsigned int src){
  union {
    unsigned int ui;
    unsigned char uc[4];
  } x,y;
  x.ui = src;
  y.uc[0] = x.uc[3];
  y.uc[1] = x.uc[2];
  y.uc[2] = x.uc[1];
  y.uc[3] = x.uc[0];
  return y.ui;
}

struct net_param get_conv(char *path){
  char buf[BUFSIZE], message[BUFSIZE];
  FILE *fp;
  int i, j, k, l, num;
  int w, x, y, z;
  struct net_param ret;

  sprintf(message, "Can't open %s", path);
  if((fp=fopen(path,"r"))==NULL)abort_stop(message);
  sprintf(message, "Can't read header of %s", path);
  if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
  sprintf(message, "Can't scan header of %s", path);
  if(sscanf(buf, "[%d,%d,%d,%d]\n", &(ret.num_conv), &(ret.x), &(ret.y), &(ret.z))!=4)abort_stop(message);

  ret.conv = (struct convolution*)malloc(sizeof(struct convolution)*ret.num_conv);
  num = 1;
  for(i=0; i<ret.num_conv; i++){
    ret.conv[i].coef = (union binary_float***)malloc(sizeof(union binary_float**)*ret.x);
    for(j=0; j<ret.x; j++){
      ret.conv[i].coef[j] = (union binary_float**)malloc(sizeof(union binary_float*)*ret.y);
      for(k=0; k<ret.y; k++){
	ret.conv[i].coef[j][k] = (union binary_float*)malloc(sizeof(union binary_float)*ret.z);
	for(l=0; l<ret.z; l++){
	  sprintf(message, "Can't read (%d) line of %s", num, path);
	  if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
	  sprintf(message, "Can't scan (%d) line of %s", num, path);
	  if(sscanf(buf, "(%d,%d,%d,%d,%x)\n", &(w), &(x), &(y), &(z), &(ret.conv[i].coef[j][k][l].a))!=5)abort_stop(message);
	  sprintf(message, "Anything wrong when scanning (%d) line of %s", num, path);
	  if(w!=i||x!=j||y!=k||z!=l)abort_stop(message);
	  num++;
	}
      }
    }
    sprintf(message, "Can't read (%d) line of %s", num, path);
    if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
    sprintf(message, "Can't scan (%d) line of %s", num, path);
    if(sscanf(buf, "(%d,%d,%d,%d,%x)\n", &(w), &(x), &(y), &(z), &(ret.conv[i].bias.a))!=5)abort_stop(message);
    sprintf(message, "Anything wrong when scanning (%d) line of %s", num, path);
    if(w!=i||x!=ret.x||y!=ret.y||z!=ret.z)abort_stop(message);
    num++;
  }
  fclose(fp);

  return ret;
}

struct net_param get_ip(char *path){
  char buf[BUFSIZE], message[BUFSIZE];
  FILE *fp;
  int i, j, num;
  int w, z;
  struct net_param ret;

  sprintf(message, "Can't open %s", path);
  if((fp=fopen(path,"r"))==NULL)abort_stop(message);
  sprintf(message, "Can't read header of %s", path);
  if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
  sprintf(message, "Can't scan header of %s", path);
  if(sscanf(buf, "[%d,%d]\n", &(ret.num_conv), &(ret.z))!=2)abort_stop(message);
  ret.x = 1; ret.y = 1;

  ret.conv = (struct convolution*)malloc(sizeof(struct convolution)*ret.num_conv);
  num = 1;
  for(i=0; i<ret.num_conv; i++){
    ret.conv[i].coef = (union binary_float***)malloc(sizeof(union binary_float**));
    ret.conv[i].coef[0] = (union binary_float**)malloc(sizeof(union binary_float*));
    ret.conv[i].coef[0][0] = (union binary_float*)malloc(sizeof(union binary_float)*ret.z);
    for(j=0; j<ret.z; j++){
      sprintf(message, "Can't read (%d) line of %s", num, path);
      if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
      sprintf(message, "Can't scan (%d) line of %s", num, path);
      if(sscanf(buf, "(%d,%d,%x)\n", &(w), &(z), &(ret.conv[i].coef[0][0][j].a))!=3)abort_stop(message);
      sprintf(message, "Anything wrong when scanning (%d) line of %s", num, path);
      if(w!=i||z!=j)abort_stop(message);
      num++;
    }
    sprintf(message, "Can't read (%d) line of %s", num, path);
    if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
    sprintf(message, "Can't scan (%d) line of %s", num, path);
    if(sscanf(buf, "(%d,%d,%x)\n", &(w), &(z), &(ret.conv[i].bias.a))!=3)abort_stop(message);
    sprintf(message, "Anything wrong when scanning (%d) line of %s", num, path);
    if(w!=i||z!=ret.z)abort_stop(message);
    num++;
  }
  fclose(fp);

  return ret;
}

void free_net_param(struct net_param src){
  int i, j, k;
  for(i=0; i<src.num_conv; i++){
    for(j=0; j<src.x; j++){
      for(k=0; k<src.y; k++){
	free(src.conv[i].coef[j][k]);
      }
      free(src.conv[i].coef[j]);
    }
    free(src.conv[i].coef);
  }
  free(src.conv);
}

struct matrix ip2matrix(struct net_param src){
  struct matrix ret;
  int i, j, num;
  num = src.num_conv * (src.z+1);

  ret.value = (union binary_float*)calloc(num, sizeof(union binary_float));
  ret.n = src.num_conv;
  ret.m = src.z+1;

  for(i=0; i<ret.n; i++){
    for(j=0; j<src.z; j++){
      ret.value[i*ret.m+j] = src.conv[i].coef[0][0][j];
    }
    ret.value[i*ret.m+src.z] = src.conv[i].bias;
  }
  return ret;
}

struct matrix setup_matrix(char *path){
  /* 
     get data of inner product (or full connection) layer as dense matrix
  */
  struct matrix ret;
  struct net_param filter;
  filter = get_ip(path);
  ret = ip2matrix(filter);
  free_net_param(filter);

  return ret;
}

void free_matrix(struct matrix src){
  free(src.value);
}

struct matrix conv2matrix(struct net_param src){
  struct matrix ret;
  int i, j, k, l;
  ret.n = src.num_conv;
  ret.m = src.x*src.y*src.z+1;
  ret.x = src.x; ret.y = src.y; ret.z = src.z;

  ret.value = (union binary_float*)calloc(ret.n*ret.m, sizeof(union binary_float));

  for(i=0; i<src.num_conv; i++){
    for(j=0; j<src.x; j++){
      for(k=0; k<src.y; k++){
	for(l=0; l<src.z; l++){
	  ret.value[i*ret.m+(j*src.y+k)*src.z+l] = src.conv[i].coef[j][k][l];
	}
      }
    }
    ret.value[i*ret.m+src.x*src.y*src.z] = src.conv[i].bias;
  }

  return ret;
}


struct matrix setup_matrix_conv(char *path){
  /* 
     get data of convolution layer as CRS (Compressed Row Storage)
     as current layer (x * yz_cur * yz_cur)
     and next layer (p * yz_next * yz_next)
  */
  struct matrix ret;
  struct net_param filter;
  filter = get_conv(path);
  ret = conv2matrix(filter);
  free_net_param(filter);

  return ret;
}

void get_meanfile(float mean[3][Y][Y], char *path){
  char buf[BUFSIZE], message[BUFSIZE];
  FILE *fp;
  int i, j, k, num;
  int x, y, z;
  union binary_float value;

  sprintf(message, "Can't open %s", path);
  if((fp=fopen(path,"r"))==NULL)abort_stop(message);
  sprintf(message, "Can't read header of %s", path);
  if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
  sprintf(message, "Can't scan header of %s", path);
  if(sscanf(buf, "[%04d,%04d,%04d]\n", &x, &y, &z)!=3)abort_stop(message);
  sprintf(message, "Anything wrong");
  if(x!=3||y!=Y||z!=Y)abort_stop(message);
  

  for(num=1, i=0; i<3; i++){
    for(j=0; j<Y; j++){
      for(k=0; k<Y; k++){
	sprintf(message, "Can't read (%d) line of %s", num, path);
	if(fgets(buf, BUFSIZE, fp)==NULL)abort_stop(message);
	sprintf(message, "Can't scan (%d) line of %s", num, path);
	if(sscanf(buf, "(%d,%d,%d,%x)\n", &(x), &(y), &(z), &(value.a))!=4)abort_stop(message);
	sprintf(message, "Anything wrong when scanning (%d) line of %s", num, path);
	if(x!=i||y!=j||z!=k)abort_stop(message);
	num++;
	mean[i][j][k] = value.f;
      }
    }
  }
  fclose(fp);
}

void get_bitmap(unsigned char image[3][Y][Y], char *path){
  Image *bitmap;
  int i, j;
  bitmap = Read_Bmp(path);
  for(i=0; i<Y; i++){
    for(j=0; j<Y; j++){
      image[0][i][j] = bitmap->data[i*Y+j].b;
      image[1][i][j] = bitmap->data[i*Y+j].g;
      image[2][i][j] = bitmap->data[i*Y+j].r;
    }
  }
  Free_Image(bitmap);
}

void free_layer(struct layer src){
  free(src.value);
}

struct layer gen_layer(int x, int y, int z, int pad){
  struct layer ret;
  ret.x = x;
  ret.y = y;
  ret.z = z;
  ret.pad = pad;
  ret.n = x*(y+2*pad)*(z+2*pad)+1;
  ret.value = (union binary_float*)calloc(ret.n, sizeof(union binary_float));
  ret.value[ret.n-1].f = 1.0f;
  return ret;
}

void MV_conv(struct layer dst, struct matrix matrix,
	     struct layer vector, int stride){
  // matrix is convolutional layer
  int i, j, k, l, m, n, index;

  for(i=0; i<dst.x; i++){
    for(j=0; j<dst.y; j++){
      for(k=0; k<dst.z; k++){
	index = (i*(dst.y+2*dst.pad)+j+dst.pad)*(dst.z+2*dst.pad)+k+dst.pad;
	dst.value[index].f = matrix.value[i*matrix.m+matrix.m-1].f;
	for(l=0; l<matrix.x; l++){
	  for(m=0; m<matrix.y; m++){
	    for(n=0; n<matrix.z; n++){
	      dst.value[index].f +=
		matrix.value[i*matrix.m+(l*matrix.y+m)*matrix.z+n].f
		* vector.value[(l*(vector.y+2*vector.pad)+j*stride+m)*(vector.z+2*vector.pad)+k*stride+n].f;
#ifdef COUNT_CALC
	      global_calc_number++;
#endif
	    }
	  }
	}
      }
    }
  }
}

void MV(struct layer dst, struct matrix matrix, struct layer vector){
  // matrix is inner product layer
  long long i, j;
  for(i=0; i<matrix.n; i++){
    dst.value[i].f = 0.0f;
    for(j=0; j<matrix.m; j++){
      /*
      printf("[i:%d,j:%d,matrix.value:%08x,matrix.column:%d]\n",
	     i, j, matrix.value[j].a, matrix.column[j]);fflush(stdout);
      */
      dst.value[i].f += matrix.value[i*matrix.m+j].f*vector.value[j].f;
#ifdef COUNT_CALC
      global_calc_number++;
#endif
    }
  }
}

void max_pooling(struct layer dst, struct layer src, int ksize, int stride){
  int i, j, k, l, m;
  union binary_float tmp;
  for(i=0; i<dst.x; i++){
    for(j=0; j<dst.y; j++){
      for(k=0; k<dst.z; k++){
	tmp = src.value[(i*(src.y+2*src.pad)+stride*j+src.pad)*(src.z+2*src.pad)+stride*k+src.pad];
	for(l=0; l<ksize; l++){
	  for(m=0; m<ksize; m++){
	    if((stride*j+l<src.y)&&(stride*k+m<src.z)){
	      if(tmp.f<src.value[(i*(src.y+2*src.pad)+stride*j+l+src.pad)*(src.z+2*src.pad)+stride*k+m+src.pad].f)
		tmp = src.value[(i*(src.y+2*src.pad)+stride*j+l+src.pad)*(src.z+2*src.pad)+stride*k+m+src.pad];
	    }
	  }
	}
	dst.value[(i*(dst.y+2*dst.pad)+j+dst.pad)*(dst.z+2*dst.pad)+k+dst.pad] = tmp;
      }
    }
  }
}

void ave_pooling(struct layer dst, struct layer src, int ksize, int stride){
  int i, j, k, l, m, count;
  union binary_float tmp;
  for(i=0; i<dst.x; i++){
    for(j=0; j<dst.y; j++){
      for(k=0; k<dst.z; k++){
	tmp.f = 0.0f;
	count = 0;
	for(l=0; l<ksize; l++){
	  for(m=0; m<ksize; m++){
	    if((stride*j+l<src.y)&&(stride*k+m<src.z)){
	      tmp.f += src.value[(i*(src.y+2*src.pad)+stride*j+l+src.pad)*(src.z+2*src.pad)+stride*k+m+src.pad].f;
	      count++;
	    }
	  }
	}
	dst.value[(i*(dst.y+2*dst.pad)+j+dst.pad)*(dst.z+2*dst.pad)+k+dst.pad].f = tmp.f/count;
      }
    }
  }
}

void ReLU(struct layer src){
  int i;
  for(i=0; i<src.n-1; i++)
    if(src.value[i].f<0.0f)
      src.value[i].f = 0.0f;
}

int maxarg(struct layer src){
  int i, ret;
  float tmp;
  ret = 0;
  tmp = src.value[0].f;
  for(i=1; i<src.z; i++){
    if(tmp<src.value[i].f){
      tmp = src.value[i].f;
      ret = i;
    }
  }
  return ret;
}

void print_output(struct layer output){
  int i;
  for(i=0; i<output.z; i++){
    printf("%14.7e ", output.value[i].f);
  }
  printf("\n");
}

struct layer input_2_layer
(unsigned char input[3][Y][Y], float mean[3][Y][Y], int y){
  int i, j, k;
  struct layer ret;
  ret = gen_layer(3, y, y, 0);
  for(i=0; i<3; i++){
    for(j=0; j<(Y<y?Y:y); j++){
      for(k=0; k<(Y<y?Y:y); k++){
	ret.value[(i*(ret.y+2*ret.pad)+j+ret.pad)*(ret.z+2*ret.pad)+k+ret.pad].f
	  = ((float)input[i][j][k])-mean[i][j][k];
      }
    }
  }
  return ret;
}

double elapsed_time(struct timeval before, struct timeval after){
  return (double)(after.tv_sec-before.tv_sec)+((double)(after.tv_usec-before.tv_usec))*1.0e-6;
}

void print_elapsed_time(struct timeval before, struct timeval after){
  printf("epalsed_time: %g [sec]\n", elapsed_time(before, after));
}

int demo_before_contraction_cognition_matrix
(unsigned char image[3][Y][Y], struct matrix conv1, struct matrix conv2, 
 struct matrix conv3, struct matrix conv4, struct matrix conv5, 
 struct matrix fc6, struct matrix fc7, struct matrix fc8, 
 float mean[3][Y][Y]){
  int ret, i;
  struct layer layer[12];
  layer[0] = input_2_layer(image, mean, 255);
  layer[1] = gen_layer(96, 62, 62, 0);
  layer[2] = gen_layer(96, 31, 31, 2);
  layer[3] = gen_layer(256, 31, 31, 0);
  layer[4] = gen_layer(256, 15, 15, 1);
  layer[5] = gen_layer(384, 15, 15, 1);
  layer[6] = gen_layer(384, 15, 15, 1);
  layer[7] = gen_layer(256, 15, 15, 0);
  layer[8] = gen_layer(256,  7,  7, 0);
  layer[9] = gen_layer(1,  1, 4096, 0);
  layer[10] = gen_layer(1, 1, 4096, 0);
  layer[11] = gen_layer(1, 1,    5, 0);

  MV_conv(layer[1], conv1, layer[0], 4); // conv1
  ReLU(layer[1]); // relu1
  max_pooling(layer[2], layer[1], 3, 2); // pool1

  MV_conv(layer[3], conv2, layer[2], 1); // conv2
  ReLU(layer[3]); // relu2
  max_pooling(layer[4], layer[3], 3, 2); // pool2

  MV_conv(layer[5], conv3, layer[4], 1); // conv3
  ReLU(layer[5]); // relu3

  MV_conv(layer[6], conv4, layer[5], 1); // conv4
  ReLU(layer[6]); // relu4

  MV_conv(layer[7], conv5, layer[6], 1); // conv5
  ReLU(layer[7]); // relu5
  max_pooling(layer[8], layer[7], 3, 2); // pool5

  MV(layer[9], fc6, layer[8]); // fc6
  ReLU(layer[9]); // relu6
  MV(layer[10], fc7, layer[9]); // fc7
  ReLU(layer[10]); // relu7
  MV(layer[11], fc8, layer[10]); // fc8

  ret = maxarg(layer[11]);
  print_output(layer[11]);
  
  for(i=0; i<12; i++){
    free_layer(layer[i]);
  }
  return ret;
}

int demo_after_contraction_cognition_matrix
(unsigned char image[3][Y][Y], struct matrix conv1, struct matrix conv2, 
 struct matrix conv3, struct matrix conv4, struct matrix fc5, 
 struct matrix fc6, float mean[3][Y][Y]){
  int ret, i;
  struct layer layer[13];
  layer[0] = input_2_layer(image, mean, 256);
  layer[1] = gen_layer(  3, 128, 128, 0);
  layer[2] = gen_layer(  3,  64,  64, 1);
  layer[3] = gen_layer( 64,  64,  64, 0);
  layer[4] = gen_layer( 64,  32,  32, 1);
  layer[5] = gen_layer(128,  32,  32, 0);
  layer[6] = gen_layer(128,  16,  16, 1);
  layer[7] = gen_layer(256,  16,  16, 0);
  layer[8] = gen_layer(256,   8,   8, 1);
  layer[9] = gen_layer(256,   8,   8, 0);
  layer[10] = gen_layer(256,   4,   4, 0);
  layer[11] = gen_layer(1, 1, 256, 0);
  layer[12] = gen_layer(1, 1,   5, 0);

  ave_pooling(layer[1], layer[0], 2, 2); // prepool1
  ave_pooling(layer[2], layer[1], 2, 2); // prepool2

  MV_conv(layer[3], conv1, layer[2], 1); // conv1
  ReLU(layer[3]); // relu1
  max_pooling(layer[4], layer[3], 2, 2); // pool1

  MV_conv(layer[5], conv2, layer[4], 1); // conv2
  ReLU(layer[5]); // relu2
  max_pooling(layer[6], layer[5], 2, 2); // pool2

  MV_conv(layer[7], conv3, layer[6], 1); // conv3
  ReLU(layer[7]); // relu3
  max_pooling(layer[8], layer[7], 2, 2); // pool3

  MV_conv(layer[9], conv4, layer[8], 1); // conv4
  ReLU(layer[9]); // relu4
  max_pooling(layer[10], layer[9], 2, 2); // pool4

  MV(layer[11], fc5, layer[10]); // fc5
  ReLU(layer[11]); // relu5
  MV(layer[12], fc6, layer[11]); // fc6

  ret = maxarg(layer[12]);
  print_output(layer[12]);
  
  for(i=0; i<13; i++){
    free_layer(layer[i]);
  }
  return ret;
}

void demo_before_contraction_classify_matrix(){
  int i, j;
  float mean[3][Y][Y];
  unsigned char image[3][Y][Y];
  char filename[BUFSIZE];
  char *label[] = {"car", "pedestrian", "sign", "light", "nothing"};
  int correct, wrong, answer;
  struct matrix conv1, conv2, conv3, conv4, conv5, fc6, fc7, fc8;

#ifdef TIME
  struct timeval a, b, c, d;

  gettimeofday(&a, NULL);
#endif

  get_meanfile(mean, "imagemean.binaryproto.txt");

  sprintf(filename, "before/conv1.txt");
  conv1 = setup_matrix_conv(filename);
  sprintf(filename, "before/conv2.txt");
  conv2 = setup_matrix_conv(filename);
  sprintf(filename, "before/conv3.txt");
  conv3 = setup_matrix_conv(filename);
  sprintf(filename, "before/conv4.txt");
  conv4 = setup_matrix_conv(filename);
  sprintf(filename, "before/conv5.txt");
  conv5 = setup_matrix_conv(filename);
  sprintf(filename, "before/fc6.txt");
  fc6 = setup_matrix(filename);
  sprintf(filename, "before/fc7.txt");
  fc7 = setup_matrix(filename);
  sprintf(filename, "before/fc8.txt");
  fc8 = setup_matrix(filename);
  
  correct = 0;
  wrong = 0;
  
#ifdef TIME
  gettimeofday(&b, NULL);
  print_elapsed_time(a, b);
#endif

  for(i=0; i<5; i++){
    for(j=0; j<70; j++){
#ifdef TIME
      gettimeofday(&c, NULL);
#endif

      sprintf(filename, "%s/%02d.bmp",
	      label[i], j);
      get_bitmap(image, filename);

      answer = demo_before_contraction_cognition_matrix
	(image, conv1, conv2, conv3, conv4, conv5, fc6, fc7, fc8, mean);
#ifdef COUNT_CALC
      printf("calc_number: %d\n", global_calc_number);
#endif

#ifdef TIME
      gettimeofday(&d, NULL);
      print_elapsed_time(c, d);
#endif
      if(answer==i){
	correct++;
#ifdef DEBUG      
	printf("O");
#endif
      }
      else{
	wrong++;
#ifdef DEBUG      
	printf("X");
#endif
      }
      fflush(stdout);
    }
  }
  printf("%g%%\n", ((double)correct/350)*100);

  free_matrix(conv1); free_matrix(conv2); free_matrix(conv3); 
  free_matrix(conv4); free_matrix(conv5);
  free_matrix(fc6); free_matrix(fc7); free_matrix(fc8);
}

#define PROC_INIT  (1)
#define PROC_RECOG (0)
#define PROC_FREE  (2)
int before_classify(int proc, unsigned char image[3][Y][Y]){
	static float mean[3][Y][Y];
	char filename[BUFSIZE];
	int answer;
	static struct matrix conv1, conv2, conv3, conv4, conv5, fc6, fc7, fc8;
	if(proc==PROC_INIT){
		get_meanfile(mean, "imagemean.binaryproto.txt");
		sprintf(filename, "before/conv1.txt");
		conv1 = setup_matrix_conv(filename);
		sprintf(filename, "before/conv2.txt");
		conv2 = setup_matrix_conv(filename);
		sprintf(filename, "before/conv3.txt");
		conv3 = setup_matrix_conv(filename);
		sprintf(filename, "before/conv4.txt");
		conv4 = setup_matrix_conv(filename);
		sprintf(filename, "before/conv5.txt");
		conv5 = setup_matrix_conv(filename);
		sprintf(filename, "before/fc6.txt");
		fc6 = setup_matrix(filename);
		sprintf(filename, "before/fc7.txt");
		fc7 = setup_matrix(filename);
		sprintf(filename, "before/fc8.txt");
		fc8 = setup_matrix(filename);
		answer = -PROC_INIT;
	}
	if(proc==PROC_RECOG){
		answer=demo_before_contraction_cognition_matrix
				(image, conv1, conv2, conv3, conv4, conv5, fc6, fc7, fc8, mean);
	}
	if(proc==PROC_FREE){
		free_matrix(conv1); free_matrix(conv2); free_matrix(conv3);
		free_matrix(conv4); free_matrix(conv5);
		free_matrix(fc6); free_matrix(fc7); free_matrix(fc8);
		answer = -PROC_FREE;
	}
	return answer;
}

int after_classify(int proc, unsigned char image[3][Y][Y]){
	static float mean[3][Y][Y];
	char filename[BUFSIZE];
	int answer;
	static struct matrix conv1, conv2, conv3, conv4, fc5, fc6;
	if(proc==PROC_INIT){
		get_meanfile(mean, "imagemean.binaryproto.txt");
		sprintf(filename, "after/conv1.txt");
		conv1 = setup_matrix_conv(filename);
		sprintf(filename, "after/conv2.txt");
		conv2 = setup_matrix_conv(filename);
		sprintf(filename, "after/conv3.txt");
		conv3 = setup_matrix_conv(filename);
		sprintf(filename, "after/conv4.txt");
		conv4 = setup_matrix_conv(filename);
		sprintf(filename, "after/fc5.txt");
		fc5 = setup_matrix(filename);
		sprintf(filename, "after/fc6.txt");
		fc6 = setup_matrix(filename);
		answer = -PROC_INIT;
	}
	if(proc==PROC_RECOG){
		answer=demo_after_contraction_cognition_matrix
				(image, conv1, conv2, conv3, conv4, fc5, fc6, mean);
	}
	if(proc==PROC_FREE){
		free_matrix(conv1); free_matrix(conv2); free_matrix(conv3);
		free_matrix(conv4); free_matrix(fc5); free_matrix(fc6);
		answer = -PROC_FREE;
	}
	return answer;
}

void demo_after_contraction_classify_matrix2(){
	int i, j;
	unsigned char image[3][Y][Y];
	char filename[BUFSIZE];
	char *label[] = {"car", "pedestrian", "sign", "light", "nothing"};
	int correct, wrong, answer;
#ifdef TIME
	struct timeval a, b, c, d; gettimeofday(&a, NULL);
#endif
	after_classify(PROC_INIT, image);
	correct = 0;
	wrong   = 0;
#ifdef TIME
	gettimeofday(&b, NULL); print_elapsed_time(a, b);
#endif
	for(i=0; i<5; i++){
		for(j=0; j<70; j++){
#ifdef TIME
			gettimeofday(&c, NULL);
#endif
			sprintf(filename, "%s/%02d.bmp",label[i], j);
			get_bitmap(image, filename);
			answer = after_classify(PROC_RECOG,image);
#ifdef TIME
			gettimeofday(&d, NULL); print_elapsed_time(c, d);
#endif
			if(answer==i){
				correct++;
#ifdef DEBUG      
				printf("O");
#endif
			}else{
				wrong++;
#ifdef DEBUG      
				printf("X[%d,%d]",i,j);
#endif
			}
			fflush(stdout);
		}
	}
	printf("%g%%\n", ((double)correct/350)*100);
	after_classify(PROC_FREE, image);
}

void demo_after_contraction_classify_matrix(){
  int i, j;
  float mean[3][Y][Y];
  unsigned char image[3][Y][Y];
  char filename[BUFSIZE];
  char *label[] = {"car", "pedestrian", "sign", "light", "nothing"};
  int correct, wrong, answer;
  struct matrix conv1, conv2, conv3, conv4, fc5, fc6;

#ifdef TIME
  struct timeval a, b, c, d;

  gettimeofday(&a, NULL);
#endif

  get_meanfile(mean, "imagemean.binaryproto.txt");

  sprintf(filename, "after/conv1.txt");
  conv1 = setup_matrix_conv(filename);
  sprintf(filename, "after/conv2.txt");
  conv2 = setup_matrix_conv(filename);
  sprintf(filename, "after/conv3.txt");
  conv3 = setup_matrix_conv(filename);
  sprintf(filename, "after/conv4.txt");
  conv4 = setup_matrix_conv(filename);
  sprintf(filename, "after/fc5.txt");
  fc5 = setup_matrix(filename);
  sprintf(filename, "after/fc6.txt");
  fc6 = setup_matrix(filename);

  correct = 0;
  wrong = 0;
  
#ifdef TIME
  gettimeofday(&b, NULL);
  print_elapsed_time(a, b);
#endif

  for(i=0; i<5; i++){
    for(j=0; j<70; j++){
#ifdef TIME
      gettimeofday(&c, NULL);
#endif

      sprintf(filename, "%s/%02d.bmp",
	      label[i], j);
      get_bitmap(image, filename);
      answer=demo_after_contraction_cognition_matrix
	(image, conv1, conv2, conv3, conv4, fc5, fc6, mean);

#ifdef COUNT_CALC
      printf("calc_number: %d\n", global_calc_number);
#endif

#ifdef TIME
      gettimeofday(&d, NULL);
      print_elapsed_time(c, d);
#endif
      if(answer==i){
	correct++;
#ifdef DEBUG      
	printf("O");
#endif
      }
      else{
	wrong++;
#ifdef DEBUG      
	printf("X[%d,%d]",i,j);
#endif
      }
      fflush(stdout);
    }
  }
  printf("%g%%\n", ((double)correct/350)*100);

  free_matrix(conv1); free_matrix(conv2); free_matrix(conv3);
  free_matrix(conv4); free_matrix(fc5); free_matrix(fc6);
}


#ifndef WITHOUT_MAIN
int main(int argc, char *argv[]){
#ifdef COUNT_CALC
  global_calc_number = 0;
#endif
  if(argc!=2){
    fprintf(stderr, "Usage: %s kind\n", argv[0]);
    exit(1);
  }
  if(Y!=256){
    fprintf(stderr, "change Y:%d\n", Y);
    exit(1);
  }

  if(strncmp(argv[1], "before", 32)==0){
    demo_before_contraction_classify_matrix();
  }
  else if(strncmp(argv[1], "after", 32)==0){
    demo_after_contraction_classify_matrix();
  }
  else {
    fprintf(stderr, "Wrong kind.\n");
    exit(1);
  }

  return EXIT_SUCCESS;
}
#endif
