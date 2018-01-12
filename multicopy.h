#ifndef __MULTI_COPY_H__
#define __MULTI_COPY_H__

#include <sys/types.h>



//传给线程函数thread_copy的结构体
struct dest_src {
	char * dest; //通过此参数告诉线程要考到哪
	char * src;     //通过此参数告诉线程，从哪考
	size_t copy_size;  //告诉线程要拷贝的大小
	size_t done_byte; //线程每拷贝一个字节，这个成员就 +1， 用于记录线程完成进度
};

//struct for process bar.
//传给线程thread_process_bar的结构体
struct process_bar_status {
    size_t copy_thread_num; //告诉process_bar线程：拷贝线程个数
    size_t file_size;
    struct dest_src * thread_copy_arg;//告诉process_bar线程：copy线程所用的参数在哪
};



//copy the src memory created from mmap() to destination also created by mmap()
void mm_cp(char * dest, const char * src, size_t size, size_t *done_byte);

//thread function for copy.
void * thread_copy(void * dest_src_arg);

//thread function for process bar.
void * thread_process_bar(void * process_bar_status);


#endif
