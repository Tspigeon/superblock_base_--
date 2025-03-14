/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName： pagemap.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description:

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/
#ifndef PAGEMAP_H
#define PAGEMAP_H 10000

#include <sys/types.h>
#include "initialize.h"

#define MAX_INT64  0x7fffffffffffffffll

void file_assert(int error,char *s);
void alloc_assert(void *p,char *s);
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope);

struct local *find_location(struct ssd_info *ssd,unsigned int ppn);
unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page);
struct ssd_info *pre_process_page(struct ssd_info *ssd);
struct ssd_info *pre_process_trace(struct ssd_info *ssd);
unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn);
struct ssd_info *get_ppn_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub);
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub);
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag);
int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node);
int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel);
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size);
unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);

int erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die,unsigned int plane ,unsigned int block);
int erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command);

Status get_blk(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id);
int find_superblock_change(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,int *blk_id);
int uninterrupt_gc_super(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int blk_id);

int move_page(struct ssd_info * ssd, struct local *location,unsigned int * transfer_size);
int gc_for_channel(struct ssd_info *ssd, unsigned int channel);
int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node);
Status dr_for_channel(struct ssd_info *ssd, unsigned int channel);
Status blk_Inqueue(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id);
Status get_GC_count_max(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id);
int uninterrupt_gc_super_soft(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int blk_id);
Status LRU_Insert(struct ssd_info *ssd, unsigned int chip, unsigned int block);
#endif


