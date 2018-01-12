
//默认mmap的空间为64MB，当然可以根据参数来指定mmap的空间大小
//bog上写明拷贝的原理，画示意图

#include "multicopy.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define DFL_MMAP_SIZE 1024*1024*64
#define MAX_THREAD 32


int main(int argc, char * argv[])
{
	char	*src  = NULL;
	char	*dest = NULL;
	int		fd_src;
	int 	fd_dest;
	size_t	thread_num = 1;
	size_t 	old_num;
	int 	err;

    //检查并且获取程序所用参数
    if(argc != 4)
	{
		printf("usage: %s src dest NUM_OF_THREAD\n", argv[0]);
		return -1;
	}
	src = argv[1];
	dest = argv[2];
	thread_num = atoi(argv[3]);
	
	if ( thread_num < 1 || thread_num > MAX_THREAD)
	{
		printf("please set the NUM_OF_THREAD between 1~%d\n", MAX_THREAD);
		return -1;
	}
	//检测要复制的文件是否存在
	if ( access(src, F_OK) < 0 )
	{
		perror(src);
		return -2;
	}
	//检测destination文件是否存在，并提示是否覆盖
	if( access(dest, F_OK) == 0 )
	{
		char ok_buf[1024];
		while(1){
			memset(ok_buf, 0, sizeof(ok_buf));
			printf("%s exists, overwrite? y/n [y]: ", dest);
			fgets(ok_buf, sizeof(ok_buf), stdin);
			if( ! strcmp(ok_buf, "\n") || ! strcmp(ok_buf, "y\n") || !strcmp(ok_buf, "n\n"))
				break;
		}
		if(strcmp(ok_buf, "n\n") == 0)
			return -3;
		remove(dest);
	}
    
	old_num = thread_num;
	
	
	
	
	int src_fd = open(src, O_RDONLY);
	if(src_fd < 0)
	{
		perror(src);
		exit(4);
	}
	
	struct stat st;
	if(fstat(src_fd, &st) < 0 )
	{
		perror("src stat");
		exit(4);
	}
	char * src_mm = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
	if(src_mm == MAP_FAILED)
	{
		perror("src map");
		exit(5);
	}
	
	int dest_fd =  open(dest, O_RDWR | O_CREAT, st.st_mode);
	if(dest_fd < 0)
	{
		perror(dest);
		exit(4);
	}
	if(lseek(dest_fd, st.st_size - 1, SEEK_SET) < 0)
	{
		perror("dest lseek");
		exit(4);
	}
	if (write(dest_fd, "\0", 1) < 0)
	{
		perror("dest write");
		exit(4);
	}
	char * dest_mm = (char *)mmap(NULL, st.st_size, PROT_WRITE, MAP_SHARED, dest_fd, 0);
	if(dest_mm == MAP_FAILED)
	{
		perror("dest map");
		exit(5);
	}
	//munmap ..... do not forget
	if(st.st_size % thread_num != 0)
		thread_num ++;
    
    //malloc a array to store dest_src_arg
    struct dest_src * dest_src_arg = (struct dest_src *)malloc(sizeof(struct dest_src)*thread_num);
    //malloc a array for tid to wait thread using this tid
    pthread_t * tid = (pthread_t *)malloc(sizeof(pthread_t)*thread_num);
    
	for(int i = 0; i < thread_num; i++)
	{
		// pass thread a struct
		dest_src_arg[i].dest 	= &dest_mm[(st.st_size/old_num) * i];
		dest_src_arg[i].src 	= &src_mm[(st.st_size/old_num) * i];
		dest_src_arg[i].done_byte = 0;
		if(old_num != thread_num)
		{
			if(i == thread_num -1)
				dest_src_arg[i].copy_size = st.st_size%old_num;
			else
				dest_src_arg[i].copy_size 	= st.st_size/old_num;
		}else
			dest_src_arg[i].copy_size 	= st.st_size/old_num;
		err = pthread_create(&tid[i], NULL, thread_copy, (void *)&dest_src_arg[i]);//way to detach??

		if(err != 0)
		{
			printf("create thread failed: %s, please try again!\n", strerror(err));
			exit(10);
		}
	}
	
	
	
	// process bar线程设置
	
	pthread_t process_bar_tid;
	
	struct process_bar_status bar_stat;
	bar_stat.copy_thread_num = thread_num;
	bar_stat.thread_copy_arg = dest_src_arg;
	bar_stat.file_size = st.st_size;
	//线程传入的参数应包括：1,线程个数，每个线程完成的字节数，拷贝文件的大小
	int err_process_bar = pthread_create(&process_bar_tid, NULL, thread_process_bar, (void *) &bar_stat);
	if( err_process_bar != 0 )
	    printf("creat thread for process faild: %s\n", strerror(err_process_bar));
	pthread_join(process_bar_tid, NULL);
	
	
	
	
	
	int join_err;
	
	for(int i = 0; i < thread_num; i++)
	{
	    join_err = pthread_join(tid[i], NULL);
	    if(join_err != 0)
	    {
	        printf("join failed: %s", strerror(join_err));
	    }
	}
	
	free(tid);
    if( munmap(src_mm, st.st_size) != 0 )
    {
        perror("src munmap failed: ");
    }
    if( munmap(dest_mm, st.st_size) != 0 )
    {
        perror("dest munmap failed: ");
    }
    
    free(dest_src_arg);
    
	return 0;
}

