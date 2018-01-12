#include "multicopy.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>



//copy the src memory created from mmap() to destination also created by mmap()
void mm_cp(char * dest, const char * src, size_t copy_size, size_t *done_byte)
{
	for(int i = 0; i < copy_size; i++)
	{
		dest[i] = src[i];
		(*done_byte)++;
	}
}

// copy thread function
void * thread_copy(void * dest_src_arg)
{
	struct dest_src * arg = (struct dest_src *)dest_src_arg;
	mm_cp(arg->dest, arg->src, arg->copy_size, &(arg->done_byte) );
	return (void *)0;
}

//thread function for process bar.
void * thread_process_bar(void * process_bar_status)
{
    
    struct winsize window;
	char * process_bar = malloc(sizeof(char)* 4096);
	struct process_bar_status * stat = (struct process_bar_status *)process_bar_status;
	
	while(1)
	{
	    ioctl(0, TIOCGWINSZ, &window);
	    unsigned short int total = window.ws_col-1;
	    memset(process_bar, 0, 4096);
	    size_t sum = 0;
        for(int i = 0; i < stat->copy_thread_num; i++)
        {
            sum += stat->thread_copy_arg[i].done_byte;
        }
        
        sprintf(process_bar, "%3ld%%", sum * 100 / stat->file_size);
        for ( int i = 0; i <= sum * (total-5)/ stat->file_size; i++)
        {
            process_bar[4+i] = '=';
            if ( i == (sum * (total-5)/ stat->file_size) )
               process_bar[4+i] = '>'; 
        }
        printf("\r%s", process_bar);
        fflush(stdout);
       
        if(sum == stat->file_size)
	        break;
        usleep(100*1000);
        
    }
    printf("\n");
    free(process_bar);
    
    return NULL;
}
