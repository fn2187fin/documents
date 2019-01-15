#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#define PROC_INIT  (1)
#define PROC_RECOG (0)
#define PROC_FREE  (2)
#define Y  (256)
extern "C" {
extern void get_bitmap(unsigned char image[3][Y][Y], char *path);
extern int after_classify(int proc, unsigned char image[3][Y][Y]);
extern int before_classify(int proc, unsigned char image[3][Y][Y]);
}

#endif
