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

#define _CRTDBG_MAP_ALLOC

#include "pagemap.h"
#include "flash.h"
#include "ssd.h"
#include <stdlib.h>


/************************************************
*当打开文件失败时，输出“open 文件名  error”
*************************************************/
void file_assert(int error,char *s)
{
    if(error == 0) return;
    printf("open %s error\n",s);
    getchar();
    exit(-1);
}

/*****************************************************
*当申请内存空间失败时，输出“malloc 变量名  error”
******************************************************/
void alloc_assert(void *p,char *s)//断言（好像是条件判断之类的）
{
    if(p!=NULL) return;
    printf("malloc %s error\n",s);
    getchar();
    exit(-1);
}

/*********************************************************************************
*断言
*A，读到的time_t，device，lsn，size，ope都<0时，输出“trace error:.....”
*B，读到的time_t，device，lsn，size，ope都=0时，输出“probable read a blank line”
**********************************************************************************/
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)//断言
{
    if(time_t <0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
    {
        printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
        getchar();
        exit(-1);
    }
    if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
    {
        printf("probable read a blank line\n");
        getchar();
    }
}


/************************************************************************************
*函数的功能是根据物理页号ppn查找该物理页所在的channel，chip，die，plane，block，page
*得到的channel，chip，die，plane，block，page放在结构location中并作为返回值
*************************************************************************************/
struct local *find_location(struct ssd_info *ssd,unsigned int ppn)
{
    struct local *location=NULL;
    unsigned int i=0;
    int pn,ppn_value=ppn;
    int page_plane=0,page_die=0,page_chip=0,page_channel=0;

    pn = ppn;

#ifdef DEBUG
    printf("enter find_location\n");
#endif

    location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
    memset(location,0, sizeof(struct local));

    page_plane=ssd->parameter->page_block*ssd->parameter->block_plane; //number of page per plane
    page_die=page_plane*ssd->parameter->plane_die;                     //number of page per die
    page_chip=page_die*ssd->parameter->die_chip;                       //number of page per chip
    page_channel=page_chip*ssd->parameter->chip_channel[0];            //number of page per channel

    /*******************************************************************************
    *page_channel是一个channel中page的数目， ppn/page_channel就得到了在哪个channel中
    *用同样的办法可以得到chip，die，plane，block，page
    ********************************************************************************/
    location->channel = ppn/page_channel;
    location->chip = (ppn%page_channel)/page_chip;
    location->die = ((ppn%page_channel)%page_chip)/page_die;
    location->plane = (((ppn%page_channel)%page_chip)%page_die)/page_plane;
    location->block = ((((ppn%page_channel)%page_chip)%page_die)%page_plane)/ssd->parameter->page_block;
    location->page = (((((ppn%page_channel)%page_chip)%page_die)%page_plane)%ssd->parameter->page_block)%ssd->parameter->page_block;

    return location;
}


/*****************************************************************************
*这个函数的功能是根据参数channel，chip，die，plane，block，page，找到该物理页号
*函数的返回值就是这个物理页号（与上面的函数是相反的操作）
******************************************************************************/
unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
    unsigned int ppn=0;
    unsigned int i=0;
    int page_plane=0,page_die=0,page_chip=0;
    int page_channel[100];                  /*这个数组存放的是每个channel的page数目，下标是channel号*/

#ifdef DEBUG
    printf("enter find_psn,channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",channel,chip,die,plane,block,page);
#endif

    /*********************************************
    *计算出plane，die，chip，channel中的page的数目，是总的跟传的参没关系吗
    **********************************************/
    page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
    page_die=page_plane*ssd->parameter->plane_die;
    page_chip=page_die*ssd->parameter->die_chip;
    while(i<ssd->parameter->channel_number)
    {
        page_channel[i]=ssd->parameter->chip_channel[i]*page_chip;
        i++;
    }

    /****************************************************************************
	*计算物理页号ppn，ppn是channel，chip，die，plane，block，page中page个数的总和
	*****************************************************************************/
    i=0;
    while(i<channel)  //当前的channel
    {
        ppn=ppn+page_channel[i];
        i++;
    }
    ppn=ppn+page_chip*chip+page_die*die+page_plane*plane+block*ssd->parameter->page_block+page;  //应该就是在物理上顺序加上

    return ppn;
}

/********************************
*函数功能是获得一个读子请求的状态
*********************************/
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size)
{
    int temp,state,move;
    if(size == 32){
        temp = 0xffffffff;
    }
    else{
        temp=~(0xffffffff<<size);  //左移size位并取反，00000111
    }
    move=lsn%ssd->parameter->subpage_page;
    state=temp<<move;  //左移move位 00011100

    return state;
}

/**************************************************
*读请求预处理函数，当读请求所读得页里面没有数据时，需要预处理往该页里面写数据，以保证能读到数据（好像是预填）
*同时也是因为要将tracefile中所有的读请求IO全部先进行优先处理
***************************************************/
struct ssd_info *pre_process_page(struct ssd_info *ssd)
{
    int fl=0;
    unsigned int device,lsn,size,ope,lpn,full_page;//IO操作的相关参数，分别是目标设备号，逻辑扇区号，操作长度，操作类型，逻辑页号
    unsigned int largest_lsn,sub_size,ppn,add_size=0;//最大的逻辑扇区号，子页操作长度，物理页号，该IO已处理完毕的长度
    unsigned int i=0,j,k;
    int map_entry_new,map_entry_old,modify;
    int flag=0;
    char buffer_request[200];//请求队列缓冲区
    struct local *location;
    int64_t time;

    printf("\n");
    printf("begin pre_process_page.................\n");

    ssd->tracefile=fopen(ssd->tracefilename,"r");//以只读方式打开trace文件，从中获取I/O请求
    if(ssd->tracefile == NULL )      /*打开trace文件从中读取请求*/
    {
        printf("the trace file can't open\n");
        return NULL;
    }
    //若读取tracefile成功，程序会先置full_page即子页屏蔽码32位都为1(初始屏蔽码默认每一个page下的所有subpage的状态都是为1)
    if(ssd->parameter->subpage_page == 32){ //16
        full_page = 0xffffffff;
    }
    else{
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page)); //有(32 - ssd->parameter->subpage_page)个前导0，后跟1。
    }
    //full_page=~(0xffffffff<<(ssd->parameter->subpage_page));

    //计算出这个ssd的最大逻辑扇区号= SSD的chip数量×每个chip下的die数量×每个die下的plane数量×每个plane下的block数量×每个block下的page数量×每个page下的subpage数量)×(1-预留空间OP比率)
    largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));

    while(fgets(buffer_request,200,ssd->tracefile))
        //逐行从tracefile文件中的所有IOtrace信息到buf缓冲区中，每次读取199（200？）字符，直到读完整个trace为止
    {
        sscanf(buffer_request,"%lld %d %d %d %d",&time,&device,&lsn,&size,&ope);
        fl++; //已读的I/O数量计数
        trace_assert(time,device,lsn,size,ope);       //进行IO正确性判断，当读到的time，device，lsn，size，ope不合法时就会处理
        ssd->total_request_num++;
        add_size=0;                        //是这个请求已经预处理的大小

        if(ope==1)        //1表示读请求           /*这里只是读请求的预处理，需要提前将相应位置的信息进行相应修改，以防越界出错等*/
        {
            while(add_size<size) //若已经预处理的操作长度<操作的长度，则执行下面操作
            {
                lsn=lsn%largest_lsn;          //防止获得的lsn比最大的lsn还大
                sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page); //= page子页数量 - lsn在page中的起始子页位置
                /*这里的sub_size主要是为了定位子请求操作位置的，这个位置是相对于某一个特定的page而言的，从这个page的第一个sub_page开始计算到这个特定的操作位置,一般是最后一页的lsn需要调整*/

                if(add_size+sub_size>=size) /*只有当一个请求的大小小于一个page的大小时或者是处理一个请求的最后一个page时会出现这种情况*/
                {
                    sub_size=size-add_size;  //=IO长度size-已经处理完毕的长度add_size
                    add_size+=sub_size;    //同时更新已经处理完毕的长度
                }

                if((sub_size>ssd->parameter->subpage_page)||(add_size>size))/*预处理一个子请求其大小大于一个page 或 已经处理的大小大于size就会报错*/
                {
                    printf("pre_process sub_size:%d\n",sub_size);
                }

                /*******************************************************************************************************
                        *利用逻辑扇区号lsn计算出逻辑页号lpn
                        *判断dram里的映射表map在lpn位置的状态
                        *A，这个状态==0，表示以前没有写过，现在需要直接将sub_size大小的子页写进去写进去（应该就仅仅打个标记）
                        *B，这个状态>0，表示，以前有写过，这需要进一步比较状态，因为新写的状态可以与以前的状态有重叠的扇区的地方
                        ********************************************************************************************************/
                lpn=lsn/ssd->parameter->subpage_page; //所在逻辑页号lpn=逻辑扇区号（逻辑子页号）lsn/一个页里的子页数
                if(ssd->dram->map->map_entry[lpn].state==0)                           /*状态为0的情况*/
                {
                    /**************************************************************
                    *获得利用get_ppn_for_pre_process函数获得ppn，再得到location
                    *修改ssd的相关参数，dram的映射表map，以及location下的page的状态
                    ***************************************************************/
                    ppn=get_ppn_for_pre_process(ssd,lsn);
                    location=find_location(ssd,ppn);
                    ssd->program_count++;
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;	//至此，表示已经成功的写入数据（读请求预处理）
                    ssd->dram->map->map_entry[lpn].pn=ppn;
                    ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001（获取的子请求状态赋值给逻辑页），sub_size是写子页的大小
                    //根据location信息更新lsn对应的物理page的相关参数
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);

                    free(location);
                    location=NULL;
                }//if(ssd->dram->map->map_entry[lpn].state==0)
                else if(ssd->dram->map->map_entry[lpn].state>0)   //说明只是有部分子页有效，而有些需要读取数据的子页并未存在dram和buffer里。所以需要从SSD中写入到dram中的子页进行相应的操作，但是必须要确认哪些子页需要进行写入。
                {
                    map_entry_new=set_entry_state(ssd,lsn,sub_size);      //得到新的状态
                    map_entry_old=ssd->dram->map->map_entry[lpn].state;   //保存当前dram中位映射信息
                    modify=map_entry_new|map_entry_old;    //更新它们相或运算后映射状态位并保存至modify
                    ppn=ssd->dram->map->map_entry[lpn].pn;    //完成映射更新后就代表程序已经完成了该lsn在当前lpn下的读操作
                    location=find_location(ssd,ppn);    //进行物理地址的查找

                    //统计相关ssd信息
                    ssd->program_count++;
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;
                    //已经从ssd中写入数据并且读到了dram中,说明映射表是在dram中的
                    ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);
                    //更新映射信息，表示已经完成了将数据写入到ssd相对应的page子页中并已经读取到了dram中
                    free(location);
                    location=NULL;
                }//else if(ssd->dram->map->map_entry[lpn].state>0)
                lsn=lsn+sub_size;                                         /*下个子请求的起始位置*/
                add_size+=sub_size;                                       /*已经处理了的add_size大小变化*/
            }//while(add_size<size)
        }//if(ope==1)
    }

    printf("\n");
    printf("pre_process is complete!\n");

    fclose(ssd->tracefile);

    //每一个plane的free空闲状态页的数量都按照固定格式写入输出文件
    for(i=0;i<ssd->parameter->channel_number;i++)
        for(j=0;j<ssd->parameter->die_chip;j++)
            for(k=0;k<ssd->parameter->plane_die;k++)
            {
                fprintf(ssd->outputfile,"chip:%d,die:%d,plane:%d have free page: %d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
                fflush(ssd->outputfile);  //每一次针对每一个plane都会用这个函数立即将缓冲区中的数据流更新写入到outputfile文件中
            }

    return ssd;
}

struct ssd_info *pre_process_trace(struct ssd_info *ssd)
{
    int fl=0;
    unsigned int device,lsn,size,ope,lpn,full_page;//IO操作的相关参数，分别是目标设备号，逻辑扇区号，操作长度，操作类型，逻辑页号
    unsigned int largest_lsn,sub_size,ppn,add_size=0;//最大的逻辑扇区号，子页操作长度，物理页号，该IO已处理完毕的长度
    unsigned int i=0,j,k;
    int map_entry_new,map_entry_old,modify;
    int flag=0;
    char buffer_request[200];//请求队列缓冲区
    struct local *location;
    int64_t time;

    printf("\n");
    printf("begin pre_process_page.................\n");

    ssd->tracefile1=fopen(ssd->tracefilename1,"r");//以只读方式打开trace文件，从中获取I/O请求
    if(ssd->tracefile1 == NULL )      /*打开trace文件从中读取请求*/
    {
        printf("the trace file can't open\n");
        return NULL;
    }
    //若读取tracefile成功，程序会先置full_page即子页屏蔽码32位都为1(初始屏蔽码默认每一个page下的所有subpage的状态都是为1)
    if(ssd->parameter->subpage_page == 32){ //16
        full_page = 0xffffffff;
    }
    else{
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page)); //有(32 - ssd->parameter->subpage_page)个前导0，后跟1。
    }
    //full_page=~(0xffffffff<<(ssd->parameter->subpage_page));

    //计算出这个ssd的最大逻辑扇区号= SSD的chip数量×每个chip下的die数量×每个die下的plane数量×每个plane下的block数量×每个block下的page数量×每个page下的subpage数量)×(1-预留空间OP比率)
    largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));

    while(fgets(buffer_request,200,ssd->tracefile1))
        //逐行从tracefile文件中的所有IOtrace信息到buf缓冲区中，每次读取199（200？）字符，直到读完整个trace为止
    {
        sscanf(buffer_request,"%lld %d %d %d %d",&time,&device,&lsn,&size,&ope);
        fl++; //已读的I/O数量计数
        trace_assert(time,device,lsn,size,ope);       //进行IO正确性判断，当读到的time，device，lsn，size，ope不合法时就会处理
        ssd->total_request_num++;
        add_size=0;                        //是这个请求已经预处理的大小

        if(ope==1)        //1表示读请求           /*这里只是读请求的预处理，需要提前将相应位置的信息进行相应修改，以防越界出错等*/
        {
            while(add_size<size) //若已经预处理的操作长度<操作的长度，则执行下面操作
            {
                lsn=lsn%largest_lsn;          //防止获得的lsn比最大的lsn还大
                sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page); //= page子页数量 - lsn在page中的起始子页位置
                /*这里的sub_size主要是为了定位子请求操作位置的，这个位置是相对于某一个特定的page而言的，从这个page的第一个sub_page开始计算到这个特定的操作位置,一般是最后一页的lsn需要调整*/

                if(add_size+sub_size>=size) /*只有当一个请求的大小小于一个page的大小时或者是处理一个请求的最后一个page时会出现这种情况*/
                {
                    sub_size=size-add_size;  //=IO长度size-已经处理完毕的长度add_size
                    add_size+=sub_size;    //同时更新已经处理完毕的长度
                }

                if((sub_size>ssd->parameter->subpage_page)||(add_size>size))/*预处理一个子请求其大小大于一个page 或 已经处理的大小大于size就会报错*/
                {
                    printf("pre_process sub_size:%d\n",sub_size);
                }

                /*******************************************************************************************************
                        *利用逻辑扇区号lsn计算出逻辑页号lpn
                        *判断dram里的映射表map在lpn位置的状态
                        *A，这个状态==0，表示以前没有写过，现在需要直接将sub_size大小的子页写进去写进去（应该就仅仅打个标记）
                        *B，这个状态>0，表示，以前有写过，这需要进一步比较状态，因为新写的状态可以与以前的状态有重叠的扇区的地方
                        ********************************************************************************************************/
                lpn=lsn/ssd->parameter->subpage_page; //所在逻辑页号lpn=逻辑扇区号（逻辑子页号）lsn/一个页里的子页数
                if(ssd->dram->map->map_entry[lpn].state==0)                           /*状态为0的情况*/
                {
                    /**************************************************************
                    *获得利用get_ppn_for_pre_process函数获得ppn，再得到location
                    *修改ssd的相关参数，dram的映射表map，以及location下的page的状态
                    ***************************************************************/
                    ppn=get_ppn_for_pre_process(ssd,lsn);
                    location=find_location(ssd,ppn);
                    ssd->program_count++;
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;	//至此，表示已经成功的写入数据（读请求预处理）
                    ssd->dram->map->map_entry[lpn].pn=ppn;
                    ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001（获取的子请求状态赋值给逻辑页），sub_size是写子页的大小
                    //根据location信息更新lsn对应的物理page的相关参数
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);

                    free(location);
                    location=NULL;
                }//if(ssd->dram->map->map_entry[lpn].state==0)
                else if(ssd->dram->map->map_entry[lpn].state>0)   //说明只是有部分子页有效，而有些需要读取数据的子页并未存在dram和buffer里。所以需要从SSD中写入到dram中的子页进行相应的操作，但是必须要确认哪些子页需要进行写入。
                {
                    map_entry_new=set_entry_state(ssd,lsn,sub_size);      //得到新的状态
                    map_entry_old=ssd->dram->map->map_entry[lpn].state;   //保存当前dram中位映射信息
                    modify=map_entry_new|map_entry_old;    //更新它们相或运算后映射状态位并保存至modify
                    ppn=ssd->dram->map->map_entry[lpn].pn;    //完成映射更新后就代表程序已经完成了该lsn在当前lpn下的读操作
                    location=find_location(ssd,ppn);    //进行物理地址的查找

                    //统计相关ssd信息
                    ssd->program_count++;
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;
                    //已经从ssd中写入数据并且读到了dram中,说明映射表是在dram中的
                    ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);
                    //更新映射信息，表示已经完成了将数据写入到ssd相对应的page子页中并已经读取到了dram中
                    free(location);
                    location=NULL;
                }//else if(ssd->dram->map->map_entry[lpn].state>0)
                lsn=lsn+sub_size;                                         /*下个子请求的起始位置*/
                add_size+=sub_size;                                       /*已经处理了的add_size大小变化*/
            }//while(add_size<size)
        }//if(ope==1)
    }

    printf("\n");
    printf("pre_process is complete!\n");

    fclose(ssd->tracefile1);

    //每一个plane的free空闲状态页的数量都按照固定格式写入输出文件
    for(i=0;i<ssd->parameter->channel_number;i++)
        for(j=0;j<ssd->parameter->die_chip;j++)
            for(k=0;k<ssd->parameter->plane_die;k++)
            {
                fprintf(ssd->outputfile,"chip:%d,die:%d,plane:%d have free page: %d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
                fflush(ssd->outputfile);  //每一次针对每一个plane都会用这个函数立即将缓冲区中的数据流更新写入到outputfile文件中
            }

    return ssd;
}

/**************************************
*函数功能是为预处理函数获取物理页号ppn
*获取页号分为动态获取和静态获取
用于将逻辑扇区号映射到对应的物理页号，以便后续的数据读取和写入操作。
**************************************/
unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn)
{
    unsigned int channel=0,chip=0,die=0,plane=0;
    unsigned int ppn,lpn;
    unsigned int active_block;
    unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;

#ifdef DEBUG
    printf("enter get_psn_for_pre_process\n");
#endif

    channel_num=ssd->parameter->channel_number;
    chip_num=ssd->parameter->chip_channel[0];
    die_num=ssd->parameter->die_chip;
    plane_num=ssd->parameter->plane_die;
    lpn=lsn/ssd->parameter->subpage_page;  //根据逻辑扇区号找到所在的逻辑页的计算公式

    /*根据lsn虚拟地址信息在程序需要申请对应的物理地址信息时对应的channel、chip、die、plane等的分配方式，即计算的公式*/

    //看根据lsn申请物理地址时channel,chip等是怎么分配的，然后反过来就知道求对应的channel,chip，die,plane
    if (ssd->parameter->allocation_scheme==0)                           /*动态方式下获取ppn*/
    {
        if (ssd->parameter->dynamic_allocation==0)                      /*表示全动态方式下，也就是channel，chip，die，plane，block等都是动态分配*/
        {
            channel=ssd->token;   //记录着当前已经分配出去供程序使用的物理channel通道号
            ssd->token=(ssd->token+1)%ssd->parameter->channel_number;  //若使用则立即更新

            chip=ssd->channel_head[channel].token; //继续往下层级申请到plane
            ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];

            die=ssd->channel_head[channel].chip_head[chip].token;
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;

            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        }
        else if (ssd->parameter->dynamic_allocation==1)                 /*表示半动态方式，channel静态按照lpn计算出，package，die，plane动态分配如上*/
        {
            channel=lpn%ssd->parameter->channel_number;
            chip=ssd->channel_head[channel].token;
            ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
            die=ssd->channel_head[channel].chip_head[chip].token;
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        }
        else if (ssd->parameter->dynamic_allocation==2)                      /*表示全动态方式下，也就是channel，chip，die，plane，block等都是动态分配*/
        {
            channel=ssd->token;   //记录着当前已经分配出去供程序使用的物理channel通道号
            ssd->token=(ssd->token+1)%ssd->parameter->channel_number;  //若使用则立即更新

            chip=ssd->channel_head[channel].token; //继续往下层级申请到plane
            ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];

            die=ssd->channel_head[channel].chip_head[chip].token;
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;

            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        }
    }
    else if (ssd->parameter->allocation_scheme==1)                       /*表示静态分配，同时也有0,1,2,3,4,5这6中不同静态分配方式*/
    {
        switch (ssd->parameter->static_allocation)
        {

            case 0:
            {
                channel=(lpn/(plane_num*die_num*chip_num))%channel_num;
                chip=lpn%chip_num;
                die=(lpn/chip_num)%die_num;
                plane=(lpn/(die_num*chip_num))%plane_num;
                break;
            }
            case 1:
            {
                channel=lpn%channel_num;
                chip=(lpn/channel_num)%chip_num;
                die=(lpn/(chip_num*channel_num))%die_num;
                plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
                break;
            }
            case 2:
            {
                channel=lpn%channel_num;
                chip=(lpn/(plane_num*channel_num))%chip_num;
                die=(lpn/(plane_num*chip_num*channel_num))%die_num;
                plane=(lpn/channel_num)%plane_num;
                break;
            }
            case 3:
            {
                channel=lpn%channel_num;
                chip=(lpn/(die_num*channel_num))%chip_num;
                die=(lpn/channel_num)%die_num;
                plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
                break;
            }
            case 4:
            {
                channel=lpn%channel_num;
                chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
                die=(lpn/(plane_num*channel_num))%die_num;
                plane=(lpn/channel_num)%plane_num;

                break;
            }
            case 5:
            {
                channel=lpn%channel_num;
                chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
                die=(lpn/channel_num)%die_num;
                plane=(lpn/(die_num*channel_num))%plane_num;

                break;
            }
            default : return 0;
        }
    }

    /******************************************************************************
    *根据上述分配方法找到channel，chip，die，plane后，再在这个里面找到active_block（每个plane里只有一个活跃块，只有再活跃块中才能进行操作）
    *接着获得ppn
    ******************************************************************************/
    if(find_active_block(ssd,channel,chip,die,plane)==FAILURE)
    {
        printf("the read operation is expand the capacity of SSD\n");
        return 0;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    if(write_page(ssd,channel,chip,die,plane,active_block,&ppn)==ERROR)
    {
        return 0;
    }

    return ppn;
}

int check_plane(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane){
    unsigned int free_page_num;
    unsigned int cumulate_free_page_num=0;
    unsigned int cumulate_free_lsb_num=0;
    unsigned int i;
    struct plane_info candidate_plane;
    candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
    for(i=0;i<ssd->parameter->block_plane;i++){
        // if(candidate_plane.blk_head[i].free_lsb_num+candidate_plane.blk_head[i].free_csb_num+candidate_plane.blk_head[i].free_msb_num!=candidate_plane.blk_head[i].free_page_num){
        // 	printf("Meta data Wrong, BLOCK LEVEL!!\n");
        // 	return FALSE;
        // 	}
        cumulate_free_page_num+=candidate_plane.blk_head[i].free_page_num;
        cumulate_free_lsb_num+=candidate_plane.blk_head[i].free_lsb_num;
    }
    if(cumulate_free_page_num!=candidate_plane.free_page){
        printf("Meta data Wrong, PLANE LEVEL!!\n");
        if(cumulate_free_page_num>candidate_plane.free_page){
            printf("The data in plane_info is SMALLER than the blocks.\n");
        }
        else{
            printf("The data in plane_info is GREATER than the blocks.\n");
        }
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************************************
*该函数实现优先寻找LSB page写入的策略
**************************************************************************************************/
struct ssd_info *get_ppn_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
    //printf("Turbo mode page allocation.\n");
    int old_ppn=-1;
    unsigned int ppn,lpn,full_page;
    unsigned int active_block;
    unsigned int block;
    unsigned int page,flag=0,flag1=0;
    unsigned int old_state=0,state=0,copy_subpage=0;
    struct local *location;
    struct direct_erase *direct_erase_node,*new_direct_erase,*direct_fast_erase_node,*new_direct_fast_erase;
    struct gc_operation *gc_node;

    unsigned int available_page;

    unsigned int i=0,j=0,k=0,l=0,m=0,n=0;

#ifdef DEBUG
    printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
    if(ssd->parameter->subpage_page == 32){
        full_page = 0xffffffff;
    }
    else{
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
    }
    lpn=sub->lpn;

    //尝试在目标plane中寻找有空闲页的block，其中优先寻找有空闲lsb page的block，否则寻找有空闲msb page的block
    if(sub->target_page_type==TARGET_LSB){
        available_page = find_active_block_lf(ssd,channel,chip,die,plane);
    }
    else if(sub->target_page_type==TARGET_CSB){
        available_page = find_active_block_csb(ssd, channel, chip, die, plane);
        if(available_page==FAILURE){
            available_page = find_active_block_lf(ssd,channel,chip,die,plane);
        }
    }
    else if(sub->target_page_type==TARGET_MSB){
        available_page = find_active_block_msb(ssd, channel, chip, die, plane);
        if(available_page==FAILURE){
            available_page = find_active_block_lf(ssd,channel,chip,die,plane);
        }
    }
    else{
        printf("WRONG Target page type!!!!\n");
        return ssd;
    }
    if(available_page == SUCCESS_LSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
        ssd->free_lsb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb;
        sub->allocated_page_type = TARGET_LSB;
    }
    else if(available_page == SUCCESS_CSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_csb_block;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
        ssd->free_csb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb;
        sub->allocated_page_type = TARGET_CSB;
    }
    else if(available_page == SUCCESS_MSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_msb_block;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->free_msb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb;
        sub->allocated_page_type = TARGET_MSB;
    }
    else{
        printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
        printf("free page number: %d, free lsb page number: %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
        return ssd;
    }
    ssd->sub_request_all++;
    if(sub->allocated_page_type == sub->target_page_type){
        ssd->sub_request_success++;
    }
    //检查页编号是否超出范围
    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=ssd->parameter->page_block){
        printf("ERROR! the last write page larger than the number of pages per block!!\n");
        printf("The active block is %d, ",active_block);
        if(available_page == SUCCESS_LSB){
            printf("targeted LSB page.\n");
        }
        else if(available_page == SUCCESS_CSB){
            printf("targeted CSB page.\n");
        }
        else{
            printf("targeted MSB page.\n");
        }
        for(i=0;i<ssd->parameter->block_plane;i++){
            printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_lsb);
        }
        printf("\n");
        for(i=0;i<ssd->parameter->block_plane;i++){
            printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_csb);
        }
        printf("\n");
        for(i=0;i<ssd->parameter->block_plane;i++){
            printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_msb);
        }
        printf("\n");
        //printf("The page number: %d.\n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
        while(1){}
    }

    //成功寻找到活跃块以及可以写入的空闲page
    block=active_block;
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;
    /*
    if(lpn == 40470){
        printf("+++++++++++++lpn:40470, location:%d, %d, %d, %d, %d, %d.\n",channel, chip, die, plane, active_block, page);
        }
    */
    //判断该逻辑页是否有对应的旧物理页，如果有就找出来并且置为无效页
    if(ssd->dram->map->map_entry[lpn].state == 0){      //lpn对应的物理页号（ppn）在映射表中的状态为0（表示无效页）
        if(ssd->dram->map->map_entry[lpn].pn != 0){
            printf("Error in get_ppn_lf()_position_1, pn: %d.\n", ssd->dram->map->map_entry[lpn].pn);
            return NULL;
        }
        //映射表中的pn字段也为0，则说明这个lpn还没有被映射到任何物理页上
        //此时，将找到一个新的物理页号，将其设置为该lpn的映射，并将映射表中该lpn对应的状态设置为sub的状态。
        ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd,channel,chip,die,plane,block,page);
        ssd->dram->map->map_entry[lpn].state = sub->state;
    }
    else{  //lpn对应的物理页号（ppn）在映射表中的状态不为0，则说明这个lpn已经有对应的物理页号映射
        ppn = ssd->dram->map->map_entry[lpn].pn;
        location = find_location(ssd,ppn);
        //检查该页存储的lpn是否与当前lpn一致
        if(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn != lpn)
        {
            printf("Error in get_ppn()_position_2\n");
            printf("lpn:%d, ppn:%d, location:%d, %d, %d, %d, %d, %d, ",lpn,ppn,location->channel,location->chip,location->die,location->plane,location->block,location->page);
            printf("the wrong lpn:%d.\n", ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn);
            return NULL;
        }
        // 如果lpn对应的物理页号在映射表中的状态不为0，并且lpn与当前映射表中的lpn一致，
        // 说明这个物理页目前是有效的，需要进行失效操作。
        // 将该物理页中存储的valid_state和free_state标记为0，表示页失效并且不再是空闲页。
        // 同时将该物理页中的lpn设置为0，表示该页不再映射到任何逻辑页上。
        // 还需要增加所在block的invalid_page_num和invalid_lsb_num
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;			  /*表示某一页失效，同时标记valid和free状态都为0*/
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;			  /*表示某一页失效，同时标记valid和free状态都为0*/
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
        if((location->page)%3==0){
            ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
        }
        /*******************************************************************************************
        *该block中全是invalid的页，可以直接删除，就在创建一个可擦除的节点，挂在location下的plane下面
        ********************************************************************************************/
        struct blk_info target_block = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block];
        if(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num == ssd->parameter->page_block)
        {
            new_direct_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
            alloc_assert(new_direct_erase,"new_direct_erase");
            memset(new_direct_erase,0, sizeof(struct direct_erase));

            new_direct_erase->block = location->block;
            new_direct_erase->next_node = NULL;
            direct_erase_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
            if(direct_erase_node == NULL){
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
            }
            else{
                new_direct_erase->next_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node = new_direct_erase;
            }
        }
        /*
        else if((target_block.free_msb_num == ssd->parameter->page_block/2)&&(target_block.invalid_lsb_num == ssd->parameter->page_block/2)){
            //这个块只有lsb被写，而且所有lsb页都是无效页
            printf("==>A FAST GC CAN BE TRIGGER.\n");
            new_direct_fast_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
            alloc_assert(new_direct_fast_erase,"new_direct_fast_erase");
            memset(new_direct_fast_erase,0, sizeof(struct direct_erase));

            new_direct_fast_erase->block = location->block;
            new_direct_fast_erase->next_node = NULL;
            direct_fast_erase_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node;
            if(direct_fast_erase_node == NULL){
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node=new_direct_fast_erase;
                }
            else{
                new_direct_fast_erase->next_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node = new_direct_fast_erase;
                }
            //ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].fast_erase = TRUE;
            printf("====>FAST GC OPT IS INITIALED.\n");
            }
            */
        free(location);
        location = NULL;
        //给这个lpn找一个新的ppn并修改其状态
        ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd,channel,chip,die,plane,block,page);
        ssd->dram->map->map_entry[lpn].state = (ssd->dram->map->map_entry[lpn].state|sub->state);
    }

    sub->ppn=ssd->dram->map->map_entry[lpn].pn; 									 /*修改sub子请求的ppn，location等变量*/
    sub->location->channel=channel;
    sub->location->chip=chip;
    sub->location->die=die;
    sub->location->plane=plane;
    sub->location->block=active_block;
    sub->location->page=page;

    ssd->program_count++;															/*修改ssd的program_count,free_page等变量*/
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    //if(page%2==0){
    //	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
    //	}
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    /*
    if(lpn == 40470){
        printf("+++++++++++++lpn:40470, stored lpn:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn);
        }
    */
    ssd->write_flash_count++;
    if(page%3 == 0){
        ssd->write_lsb_count++;
        ssd->newest_write_lsb_count++;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
        //ssd->free_lsb_count--;
    }
    else if(page%3 == 1){
        ssd->write_csb_count++;
        ssd->newest_write_csb_count++;
    }
    else{
        ssd->write_msb_count++;
        ssd->newest_write_msb_count++;
    }


    //如果空闲页低于gc阈值，就生成一个gc节点，准备进行gc操作
    if (ssd->parameter->active_write == 0)											/*如果没有主动策略，只采用gc_hard_threshold，并且无法中断GC过程*/
    {																				/*如果plane中的free_page的数目少于gc_hard_threshold所设定的阈值就产生gc操作*/
        if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page < (ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
        {
            //printf("+++++Garbage Collection is NEEDED.\n");
            gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
            alloc_assert(gc_node,"gc_node");
            memset(gc_node,0, sizeof(struct gc_operation));

            gc_node->next_node=NULL;
            gc_node->chip=chip;
            gc_node->die=die;
            gc_node->plane=plane;
            gc_node->block=0xffffffff;
            gc_node->page=0;
            gc_node->state=GC_WAIT;
            gc_node->priority=GC_UNINTERRUPT;
            gc_node->next_node=ssd->channel_head[channel].gc_command;
            ssd->channel_head[channel].gc_command=gc_node;
            ssd->gc_request++;
        }
            /*IMPORTANT!!!!!!*/
            /*Fast Collection Function*/
        else if(ssd->parameter->fast_gc == 1){
            if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num < (ssd->parameter->page_block*ssd->parameter->block_plane/2)*ssd->parameter->fast_gc_thr_2){
                //printf("==>A FAST GC CAN BE TRIGGERED, %d,%d,%d,%d,,,%d.\n",channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
                struct gc_operation *temp_gc_opt;
                int temp_flag=0;
                temp_gc_opt = ssd->channel_head[channel].gc_command;
                while(temp_gc_opt != NULL){
                    if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
                        temp_flag=1;
                        break;
                    }
                    temp_gc_opt = temp_gc_opt->next_node;
                }
                if(temp_flag!=1){
                    gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
                    alloc_assert(gc_node,"fast_gc_node");
                    memset(gc_node,0, sizeof(struct gc_operation));

                    gc_node->next_node=NULL;
                    gc_node->chip=chip;
                    gc_node->die=die;
                    gc_node->plane=plane;
                    gc_node->block=0xffffffff;
                    gc_node->page=0;
                    gc_node->state=GC_WAIT;
                    gc_node->priority=GC_FAST_UNINTERRUPT_EMERGENCY;
                    gc_node->next_node=ssd->channel_head[channel].gc_command;
                    ssd->channel_head[channel].gc_command=gc_node;
                    ssd->gc_request++;
                }
            }
            else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num < (ssd->parameter->page_block*ssd->parameter->block_plane/2)*ssd->parameter->fast_gc_thr_1){
                //printf("==>A FAST GC CAN BE TRIGGERED, %d,%d,%d,%d,,,%d.\n",channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
                struct gc_operation *temp_gc_opt;
                int temp_flag=0;
                temp_gc_opt = ssd->channel_head[channel].gc_command;
                while(temp_gc_opt != NULL){
                    if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
                        temp_flag=1;
                        break;
                    }
                    temp_gc_opt = temp_gc_opt->next_node;
                }
                if(temp_flag!=1){
                    gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
                    alloc_assert(gc_node,"fast_gc_node");
                    memset(gc_node,0, sizeof(struct gc_operation));

                    gc_node->next_node=NULL;
                    gc_node->chip=chip;
                    gc_node->die=die;
                    gc_node->plane=plane;
                    gc_node->block=0xffffffff;
                    gc_node->page=0;
                    gc_node->state=GC_WAIT;
                    gc_node->priority=GC_FAST_UNINTERRUPT;
                    gc_node->next_node=ssd->channel_head[channel].gc_command;
                    ssd->channel_head[channel].gc_command=gc_node;
                    ssd->gc_request++;
                }
            }
            else if(ssd->request_queue->priority==0){
                //printf("It is IDLE time for garbage collection.\n")
                struct gc_operation *temp_gc_opt;
                int temp_flag=0;
                temp_gc_opt = ssd->channel_head[channel].gc_command;
                /*
                while(temp_gc_opt != NULL){
                    //修改为每个chip最多有一个回收，避免过多回收影响性能
                    //if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
                    if(temp_gc_opt->chip==chip){
                        temp_flag=1;
                        break;
                        }
                    temp_gc_opt = temp_gc_opt->next_node;
                    }
                    */
                //要求每次进行gc的channel数量不超过总channel数的25%
                unsigned int channel_count,gc_channel_num = 0;
                unsigned int newest_avg_write_lat;
                if(ssd->newest_write_request_count==0){
                    newest_avg_write_lat = 0;
                }
                else{
                    newest_avg_write_lat = ssd->newest_write_avg/ssd->newest_write_request_count;
                }
                if(newest_avg_write_lat >= 8000000){
                    /*
                    unsigned int last_ten_write_avg_lat=0;
                    unsigned int fgc_i;
                    for(fgc_i=0;fgc_i<10;fgc_i++){
                        last_ten_write_avg_lat += ssd->last_ten_write_lat[fgc_i];
                        }
                    last_ten_write_avg_lat = last_ten_write_avg_lat/10;
                    if(last_ten_write_avg_lat >= 8000000){
                    */
                    temp_flag=1;
                }
                else if(temp_gc_opt == NULL){
                    for(channel_count=0;channel_count<ssd->parameter->channel_number;channel_count++){
                        if(ssd->channel_head[channel_count].gc_command != NULL){
                            gc_channel_num++;
                        }
                    }
                    if(gc_channel_num >= 4){
                        temp_flag=1;
                    }
                }
                else{
                    temp_flag=1;
                }
                if(temp_flag!=1){
                    gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
                    alloc_assert(gc_node,"fast_gc_node");
                    memset(gc_node,0, sizeof(struct gc_operation));

                    gc_node->next_node=NULL;
                    gc_node->chip=chip;
                    gc_node->die=die;
                    gc_node->plane=plane;
                    gc_node->block=0xffffffff;
                    gc_node->page=0;
                    gc_node->state=GC_WAIT;
                    gc_node->priority=GC_FAST_UNINTERRUPT_IDLE;
                    gc_node->next_node=ssd->channel_head[channel].gc_command;
                    ssd->channel_head[channel].gc_command=gc_node;
                    ssd->gc_request++;
                }
            }
        }
    }
    if(check_plane(ssd, channel, chip, die, plane) == FALSE){
        printf("Something Wrong Happened.\n");
        return FAILURE;
    }
    return ssd;
}

/***************************************************************************************************
*函数功能是在所给的channel，chip，die，plane里面找到一个active_block，然后再在这个block里面找到一个页，再利用find_ppn找到ppn。
可能产生擦除节点挂载plane上，可能产生GC节点挂载channel上。
****************************************************************************************************/
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)  //
{
    //如果是lsb优先策略，那直接调用get_ppn_lf
    if(ssd->parameter->lsb_first_allocation== 1){
        return get_ppn_lf(ssd,channel,chip,die,plane,sub);

        //if((rand()%100)<ssd->parameter->turbo_mode_factor){
        //printf("turbo write\n...");
        //return get_ppn_lf(ssd,channel,chip,die,plane,sub);

        //}
    }

    //下面的就跟get_ppn_lf后半段一样了，找活跃块，找lpn对应的ppn，标记失效页，新增gc节点等等
    int old_ppn=-1;
    unsigned int ppn,lpn,full_page;
    unsigned int active_block;
    unsigned int block;
    unsigned int page,flag=0,flag1=0;
    unsigned int old_state=0,state=0,copy_subpage=0;
    struct local *location;
    struct direct_erase *direct_erase_node,*new_direct_erase;
    struct gc_operation *gc_node;    

    unsigned int i=0,j=0,k=0,l=0,m=0,n=0;

#ifdef DEBUG
    printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
    if(ssd->parameter->subpage_page == 32){
        full_page = 0xffffffff;
    }
    else{
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page)); //左移ssd->parameter->subpage_page位之后取反
    }
    lpn=sub->lpn;

    /*************************************************************************************
    *利用函数find_active_block在channel，chip，die，plane找到活跃block
    *并且修改这个channel，chip，die，plane，active_block下的last_write_page和free_page_num
    **************************************************************************************/
    if(find_active_block(ssd,channel,chip,die,plane)==FAILURE)
    {
        printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
        return ssd;
    }

    //找到了active block
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;  //好像是写了一次那个页
    //last_write_page是从0开始的，每次进这个都会从active_block上进行++

    //******************************************Modification for turbo_mode*********************************
    /*
    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page%3 == 0){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb += 3;
        }
    else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page%3 == 1){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb += 3;
        }
    else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page%3 == 2){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb += 3;
        }
    else{
        printf("Not gonna happen.\n");
        }
        */
    /*
    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb+3;
        }
    else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb+3;
        }
    else{
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb+3;
        }
        */
    //******************************************
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;  //空闲页--，但也没说明写好了

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=ssd->parameter->page_block)
    {
        printf("%d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase);
        printf("%d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block+1].fast_erase);
        printf("%d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num);
        printf("%d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block+1].free_page_num);
        printf("error! the last write page larger than the number of pages per block!! %d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
        while(1){}
    }

    //write++，page--就只是做了一个标记，以下开始写！！这个重要

    block=active_block;  //我们要写上去的block就是这个block
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;
    //TLC里面有三种页
    if(page%3==0){  //就说明是LSB
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb=page;
        ssd->free_lsb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
        //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->write_lsb_count++;
        ssd->newest_write_lsb_count++;
        //**********************
        sub->allocated_page_type = TARGET_LSB;
        //**********************
    }
    else if(page%3==2){  //就说明是MSB
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb=page;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
        //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->write_msb_count++;
        ssd->free_msb_count--;
        ssd->newest_write_msb_count++;
        //**********************
        sub->allocated_page_type = TARGET_MSB;
        //**********************
    }
    else{    //就说明是CSB
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb=page;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
        ssd->write_csb_count++;
        ssd->free_csb_count--;
        ssd->newest_write_csb_count++;
        sub->allocated_page_type = TARGET_CSB;
    }
    /*至此，物理地址已找到，现在要做的就是建立映射
    首先，看这个物理地址是不是已经有数据了，
    若没有，直接把物理地址转换成ppn，然后建立lpn与ppn之间的映射关系即可
    若已经有数据了，就需原来的数据置为失效，现有的请求写到其他位置
    */
    if(ssd->dram->map->map_entry[lpn].state==0)     //没有映射关系，表示该子请求不是更新请求，可以直接写入      /*this is the first logical page*/
    {
        if(ssd->dram->map->map_entry[lpn].pn!=0)
        {
            printf("Error in get_ppn()\n");
        }
        ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);  //找到一个空闲页获得ppn，并更新映射表
        ssd->dram->map->map_entry[lpn].state=sub->state;
        ssd->write_req_True++;
    }
    else      //此时有映射关系，就说明对这个逻辑页进行了更新，需要将原来的页置为失效
    {
        ssd->write_req_True++;
        ppn=ssd->dram->map->map_entry[lpn].pn;
        location=find_location(ssd,ppn);
        if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!=lpn)
        {
            printf("\nError in get_ppn()\n");
        }
        //找到了location过后就会把这个页置为无效
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;          //表示其中某一页失效，同时标记valid和free状态都为0
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;         //表示某一页失效，同时标记valid和free状态都为0
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;   //删除该页的映射
        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;   //无效页++
        // SB的invalid_page++
        ssd->superblock[location->block+(location->chip*ssd->parameter->block_plane)].invalid_page_count++;
        // 并将SB加入lru列表
        // 创建节点
        // lru_node=(struct CacheNode *)malloc(sizeof(struct CacheNode));
        // alloc_assert(lru_node,"lru_node");
        // memset(lru_node,0, sizeof(struct CacheNode));
        // lru_node->superblockID = (location->block)+(location->chip*ssd->parameter->block_plane);
        // lru_node->next = NULL;
        // if(ssd->Lruhead == NULL){   // 第一个插入
        //     ssd->Lrutail = ssd->Lruhead = lru_node;
        //     ssd->Lrusize++;
        // }else if(ssd->Lrucapacity > ssd->Lrusize){     //lru未满，直接添加 尾插
        //     struct CacheNode *pre_lruNode = ssd->Lruhead;
        //     if(ssd->Lruhead->superblockID == lru_node->superblockID){   // 头节点就是
        //         ssd->Lruhead = ssd->Lruhead->next;
        //     }else{  // 头节点不是，跳过头处理
        //         while(pre_lruNode->next != NULL){
        //             if(pre_lruNode->next->superblockID == lru_node->superblockID){    // 有重复的SBblkID
        //                 // 当前的下一个重复了，删除下一个
        //                 pre_lruNode->next = pre_lruNode->next->next;
        //                 ssd->Lrusize--;
        //                 if(pre_lruNode->next == NULL){
        //                     // 迁移tail指针
        //                     ssd->Lrutail = pre_lruNode;
        //                 }
        //                 break;
        //             }
        //             pre_lruNode=pre_lruNode->next;
        //         }
        //     }
        //     ssd->Lrutail->next = lru_node;
        //     ssd->Lrutail = lru_node;
        //     ssd->Lrusize++;

        // }else if(ssd->Lrucapacity <= ssd->Lrusize){
        //     int is_rep = 0;
        //     struct CacheNode *pre_lruNode = ssd->Lruhead;
        //     if(ssd->Lruhead->superblockID == lru_node->superblockID){   // 头节点就是
        //         ssd->Lruhead = ssd->Lruhead->next;
        //         is_rep = 1;
        //     }else{  // 头节点不是，跳过头处理
        //         while(pre_lruNode->next != NULL){
        //             if(pre_lruNode->next->superblockID == lru_node->superblockID){    // 有重复的SBblkID
        //                 // 当前的下一个重复了，删除下一个
        //                 pre_lruNode->next = pre_lruNode->next->next;
        //                 is_rep = 1;
        //                 if(pre_lruNode->next == NULL){
        //                     // 迁移tail指针
        //                     ssd->Lrutail = pre_lruNode;
        //                 }
        //                 break;
        //             }
        //             pre_lruNode=pre_lruNode->next;
        //         }
        //     }
        //     //lru满了，先头出再尾插
        //     if(is_rep == 0){    //且没有重复
        //         struct CacheNode *temp = ssd->Lruhead;
        //         ssd->Lruhead = temp->next;
        //         free(temp);
        //     }
        //     ssd->Lrutail->next = lru_node;
        //     ssd->Lrutail = lru_node;

        // }

        if(LRU_Insert(ssd, location->chip, location->block) == FALSE){
            printf("ERROR in LRU Insert. chip is %d, block is %d", location->chip, location->block);
        }

        if((location->page)%3==0){
            ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
        }
        /*******************************************************************************************
        ***该block中全是invalid的页，可以直接擦除整个block，就再创建一个可擦除的节点，挂在location下的plane下面
        ********************************************************************************************/
        // if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)  //如果这个块的无效页等于 这个？总页数说明块整个都无效了
        // {
        // 	//进行直接gc
        // 	new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));  //创建一个直接gc的节点
        //     alloc_assert(new_direct_erase,"new_direct_erase");
        // 	memset(new_direct_erase,0, sizeof(struct direct_erase));

        // 	new_direct_erase->block=location->block;
        // 	new_direct_erase->next_node=NULL;
        // 	direct_erase_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
        // 	if (direct_erase_node==NULL)
        // 	{
        // 		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
        // 	}
        // 	else
        // 	{
        // 		new_direct_erase->next_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
        // 		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
        // 	}
        // }

        free(location);
        location=NULL;
        ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
        ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->state);
    }


    sub->ppn=ssd->dram->map->map_entry[lpn].pn;                                      /*修改sub子请求的ppn，location等变量*/
    sub->location->channel=channel;
    sub->location->chip=chip;
    sub->location->die=die;
    sub->location->plane=plane;
    sub->location->block=active_block;
    sub->location->page=page;

    ssd->program_count++;                                                           /*修改ssd的program_count,free_page等变量*/
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    ssd->write_flash_count++;

    if (ssd->parameter->active_write==0)  /*如果没有主动策略，只采用gc_hard_threshold，并且无法中断GC过程*/
    {
        // 硬阈值
        if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))  //如果plane中的free_page有效页的数目少于gc_hard_threshold所设定的阈值就产生gc操作
        {
            ssd->hard_th_freepg = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page + ssd->hard_th_freepg;
            int blk_id = -1;
            // 原函数为选出无效页最多的块，现在要选出已经被gc最多的块
            if (get_GC_count_max(ssd, channel, chip, die, plane, &blk_id) == ERROR) {
                printf("Error: no blk for gc_count_max %d \n", blk_id);
                getchar();
            }
            for(int i = 0;i < ssd->parameter->channel_number;i++)
            {
                int blk_number = -1;
                // 因为传的是超级块号，所以得返回没有gc的块号
                blk_number = ssd->superblock[blk_id].super_blk_loc[i].blk;
                // 判断这个块有没有被page_move
                if(ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[blk_number].SB_gc_flag == 1)
                {
                    //printf("find %d already gc and %d\n", blk_number, ssd->superblock[blk_number+(chip*ssd->parameter->block_plane)].gc_count);
                    continue;
                }
                gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
                alloc_assert(gc_node,"gc_node");
                memset(gc_node,0, sizeof(struct gc_operation));

                gc_node->next_node=NULL;
                gc_node->chip=chip;
                gc_node->die=die;
                gc_node->plane=plane;
                gc_node->block=blk_number;
                gc_node->page=0;
                gc_node->state=GC_WAIT;
                gc_node->priority=GC_UNINTERRUPT;
                gc_node->type=1; //1为硬阈值
                ssd->hard_count++; //硬阈值统计值++

                // 删除之前放入的gc节点
                if(ssd->superblock[blk_number+(chip*ssd->parameter->block_plane)].is_softSB_inQue == 1){
                    struct gc_operation *gc_pre=NULL;
                    if(ssd->channel_head[i].gc_command != NULL){
                        gc_pre=ssd->channel_head[i].gc_command;
                        if(gc_pre->chip == chip && gc_pre->block == blk_number){ //第一个节点就是要删除的
                            if(gc_pre->next_node == NULL){  //最后一个节点
                                ssd->channel_head[i].gc_command = ssd->channel_head[i].gc_command_tail = NULL; 
                            }else{  //不是最后一个节点
                                ssd->channel_head[i].gc_command = gc_pre->next_node;
                            }
                            ssd->gc_request--;
                        }else{
                            while (gc_pre->next_node!=NULL)
                            {
                                if ((gc_pre->next_node->chip == chip) && (gc_pre->next_node->block == blk_number))
                                {
                                    gc_pre->next_node=gc_pre->next_node->next_node;
                                        ssd->gc_request--; 
                                        if(gc_pre->next_node == NULL){
                                            ssd->channel_head[i].gc_command_tail = gc_pre;
                                        }
                                        break;
                                }
                                    gc_pre=gc_pre->next_node;
                                }
                        }
                    }
                }



                //第一个插入的
                if(ssd->channel_head[i].gc_command == NULL ){
                    ssd->channel_head[i].gc_command = gc_node;
                    ssd->channel_head[i].gc_command_tail = gc_node;
                }else{
                    gc_node->next_node=ssd->channel_head[i].gc_command;
                    ssd->channel_head[i].gc_command=gc_node;    //接在这个channel的gc链表后面，应该是链表的头插操作
                }
                ssd->gc_request++;
                if(ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[blk_number].free_page_num > 0 )
                {
                    printf("---------------\n");
                    while (1){}
                }
            }
        }

        // 如果硬阈值没有满足，则测试软阈值
        if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_threshold) && ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page>(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold) && (ssd->gc_request < 1024))  //如果plane中的free_page有效页的数目少于gc_threshold所设定的阈值就产生gc操作
        {
            ssd->soft_th_freepg = ssd->soft_th_freepg+ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page;
            int blk_id = -1;
            if (blk_Inqueue(ssd, channel, chip, die, plane, &blk_id) == ERROR) {
                // printf("Error: no blk for blkInqueue\n");
                // getchar();
                return ssd;
            }
            // 说明有软阈值块
            // 将软阈值块标志位置1
            ssd->superblock[blk_id].is_softSB_inQue = 1;
            ssd->cold_choose++;
            for(int i = 0;i < ssd->parameter->channel_number;i++)
            {
                int gc_change_block = -1;
                //因为传的是超级块号，所以得返回具体块号,gc_change_blk是小块号
                gc_change_block = find_superblock_change(ssd,i,chip,die,plane,&blk_id);

                if(gc_change_block == ERROR)
                {
                    printf("Error: no find blk\n");
                    getchar();
                }

                // 循环channel，把小块放入gc队列
                gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
                alloc_assert(gc_node,"gc_node");
                memset(gc_node,0, sizeof(struct gc_operation));

                gc_node->next_node=NULL;
                gc_node->chip=chip;
                gc_node->die=die;
                gc_node->plane=plane;
                gc_node->block=gc_change_block;
                gc_node->page=0;
                gc_node->state=GC_WAIT;
                gc_node->priority=GC_UNINTERRUPT;
                gc_node->type = 0; //0代表软


                //在软阈值处加入空闲条件判断
                // unsigned int current_state = 0, next_state = 0;
                // long long next_state_predict_time = 0;
                // current_state=ssd->channel_head[i].chip_head[gc_node->chip].current_state;
                // next_state=ssd->channel_head[i].chip_head[gc_node->chip].next_state;
                // next_state_predict_time=ssd->channel_head[i].chip_head[gc_node->chip].next_state_predict_time;
                // if((ssd->gc_request < 16))
                //     {
                //         ssd->superblock[blk_id].is_softSB_inQue = 1;
                //         // 软阈值这里进行尾插
                //         if(ssd->channel_head[i].gc_command == NULL || ssd->channel_head[i].gc_command_tail == NULL){
                //             ssd->channel_head[i].gc_command = gc_node;
                //             ssd->channel_head[i].gc_command_tail = gc_node;
                //         }else{
                //             ssd->channel_head[i].gc_command_tail->next_node = gc_node;
                //             ssd->channel_head[i].gc_command_tail = gc_node;
                //         }
                //         ssd->gc_request++;
                //     }
                //end  

                // 软阈值这里进行尾插
                if(ssd->channel_head[i].gc_command == NULL || ssd->channel_head[i].gc_command_tail == NULL){
                    ssd->channel_head[i].gc_command = gc_node;
                    ssd->channel_head[i].gc_command_tail = gc_node;
                }else{
                    ssd->channel_head[i].gc_command_tail->next_node = gc_node;
                    ssd->channel_head[i].gc_command_tail = gc_node;
                }
                ssd->gc_request++;
                 if(ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_change_block].free_page_num > 0 )
                 {
                 	printf("soft---------------\n");
                 	while (1){}
                 }
            }
        }
    }
    if(check_plane(ssd, channel, chip, die, plane) == FALSE){
        printf("Something Wrong Happened.\n");
        return FAILURE;
    }
    return ssd;
}

int find_superblock_change(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,int *blk_id)
{
    unsigned int block = *blk_id;
    unsigned int current_block = ssd->superblock[block].super_blk_loc[channel].blk;
//    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[current_block].super_flag = 1;
    return current_block;
}



Status get_blk(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id){
    int i;
    int block = -1;
    int active_block = 0; // 默认活跃块号
    unsigned int invalid_page = 0;
    unsigned int superblock_invalid_page_num = 0;

    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)     /*获取活跃块*/
    {
        printf("\n\n Error in uninterrupt_gc().\n");
        return ERROR;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    int start_superblock = (chip == 0) ? 0 : ssd->parameter->block_plane;
    int end_superblock = (chip == 0) ? ssd->parameter->block_plane : ssd->parameter->block_plane * ssd->parameter->chip_channel[0];
    //遍历的是超级块0-ssd->parameter->block_plane
    for(i=start_superblock;i<end_superblock;i++)
    {
        int flag = 0;
        for(int j = 0;j < ssd->parameter->channel_number;j++)
        {
            if(ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].free_page_num > 0
               || ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].super_flag == 1)
            {
                flag = 1;
                break;
            }
            if(j == channel)
            {
                continue;
            }
        }
        if(flag == 1)
        {
            continue;
        }
        superblock_invalid_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[channel].blk].invalid_page_num;
        if((active_block!=i)&&(superblock_invalid_page_num>invalid_page))
        {
            invalid_page=superblock_invalid_page_num;
            block=i;
        }
    }
    *blk_id = block;
    if (block == -1)
    {
        return ERROR;
    }
    return SUCCESS;
}

// 当无效页到达硬阈值的时候，将gc_count最多的块插到对头进行gc，这里选择gc_count最多的块，返回SBid
Status get_GC_count_max(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id){
    int i;
    int block = -1;
    int active_block = 0; // 默认活跃块号
    unsigned int gc_count_max = 0; // 当前最多的gc_count
    unsigned int invalid_page = 0;
    unsigned int superblock_invalid_page_num = 0;


    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)     /*获取活跃块*/
    {
        printf("\n\n Error in uninterrupt_gc().\n");
        return ERROR;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    int start_superblock = (chip == 0) ? 0 : ssd->parameter->block_plane;
    int end_superblock = (chip == 0) ? ssd->parameter->block_plane : ssd->parameter->block_plane * ssd->parameter->chip_channel[0];
    //遍历的是超级块0-ssd->parameter->block_plane
    for(i=start_superblock;i<end_superblock;i++)
    {
        int flag = 0;
        for(int j = 0;j < ssd->parameter->channel_number;j++)
        {
            superblock_invalid_page_num += ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].invalid_page_num;
            if(ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].free_page_num > 0
               || ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].super_flag == 1)
            {
                flag = 1;
                break;
            }
            if(j == channel)
            {
                continue;
            }
        }
        if(flag == 1)
        {
            continue;
        }
        // printf("%d and %d and %d \n", i, ssd->superblock[i].gc_count, ssd->parameter->channel_number);
        // superblock_invalid_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[channel].blk].invalid_page_num;
//        if((active_block!=i)&&(ssd->superblock[i].gc_count >= gc_count_max)&&(ssd->superblock[i].gc_count < ssd->parameter->channel_number))
//        {
//            gc_count_max=ssd->superblock[i].gc_count;
//            block=i;
//        }   
        // superblock_invalid_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[channel].blk].invalid_page_num;
        if((active_block!=i)&&(superblock_invalid_page_num>invalid_page))
        {
            invalid_page=superblock_invalid_page_num;
            block=i;
        }
        superblock_invalid_page_num = 0;
    }
    for(i = 0;i < ssd->parameter->channel_number; i++)
    {
        ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[block].super_blk_loc[i].blk].super_flag = 1;
    }
    *blk_id = block;
    if (block == -1)
    {
        printf("%d and %d and %d\n", gc_count_max, ssd->superblock[i].gc_count, ssd->parameter->channel_number);
        return ERROR;
    }
    return SUCCESS;
}

// 当无效页到达软阈值的时候，选择超级块软阈值放入队列，返回的是超级块号
Status blk_Inqueue(struct ssd_info *ssd, int channel, int chip, int die, int plane, int *blk_id){
    int i;
    int block = -1;
    int active_block = 0; // 默认活跃块号
    unsigned int invalid_page = 0;
    unsigned int superblock_invalid_page_num = 0;

    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)     /*获取活跃块*/
    {
        printf("\n\n Error in uninterrupt_gc().\n");
        return ERROR;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    // 超级块起始和结束
    int start_superblock = (chip == 0) ? 0 : ssd->parameter->block_plane;
    int end_superblock = (chip == 0) ? ssd->parameter->block_plane : ssd->parameter->block_plane * ssd->parameter->chip_channel[0];
    //遍历的是超级块0-ssd->parameter->block_plane
    for(i=start_superblock;i<end_superblock;i++)
    {
        int flag = 0;
        for(int j = 0;j < ssd->parameter->channel_number;j++)
        {
            superblock_invalid_page_num += ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].invalid_page_num;
            if(ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].free_page_num > 0
               || ssd->channel_head[j].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[j].blk].super_flag == 1)
            {
                flag = 1;
                break;
            }
            if(j == channel)
            {
                continue;
            }
        }
        if(flag == 1)
        {
            continue;
        }
        
        // superblock_invalid_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[channel].blk].invalid_page_num;
              
        superblock_invalid_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->superblock[i].super_blk_loc[channel].blk].invalid_page_num;
        if( (active_block!=i) && (superblock_invalid_page_num > invalid_page) && (ssd->superblock[i].is_softSB_inQue == 0)) // 不是忙碌快，而且无效页超过阈值,而且不再队列中
        {
            // 判断冷热
            // if(ssd->superblock[i].superblock_erase < 20){
            // 选择block, 防止重复被选中
            // 准备加入一个lru链表，判断冷热
            struct CacheNode *lru_srh = ssd->Lruhead;
            int lru_flag = 1;   // 是否在lru链表中找到该块
            while(lru_srh != NULL){
                if(lru_srh->superblockID == i){
                    // 说明为热块，不采用
                    lru_flag = 0;
                }
                lru_srh = lru_srh->next;
            }
            if(lru_flag == 1){   // 说明为冷块
                invalid_page = superblock_invalid_page_num;
                block = i; 
            }
            // invalid_page = superblock_invalid_page_num;
            // block = i; 
        }
        superblock_invalid_page_num = 0;
    }
    *blk_id = block;
    if (block == -1)
    {
        return ERROR;
    }
    return SUCCESS;
}

// LRU管理函数,insert
Status LRU_Insert(struct ssd_info *ssd, unsigned int chip, unsigned int block){
        struct CacheNode *lru_node;
        // 并将SB加入lru列表
        // 创建节点
        lru_node=(struct CacheNode *)malloc(sizeof(struct CacheNode));
        alloc_assert(lru_node,"lru_node");
        memset(lru_node,0, sizeof(struct CacheNode));
        lru_node->superblockID = (block)+(chip*ssd->parameter->block_plane);
        lru_node->next = NULL;
        if(ssd->Lruhead == NULL){   // 第一个插入
            ssd->Lrutail = ssd->Lruhead = lru_node;
            ssd->Lrusize++;
            return TRUE;
        }else if(ssd->Lrucapacity > ssd->Lrusize){     //lru未满，直接添加 尾插
            struct CacheNode *pre_lruNode = ssd->Lruhead;
            if(ssd->Lruhead->superblockID == lru_node->superblockID){   // 头节点就是
                ssd->Lruhead = ssd->Lruhead->next;
            }else{  // 头节点不是，跳过头处理
                while(pre_lruNode->next != NULL){
                    if(pre_lruNode->next->superblockID == lru_node->superblockID){    // 有重复的SBblkID
                        // 当前的下一个重复了，删除下一个
                        pre_lruNode->next = pre_lruNode->next->next;
                        ssd->Lrusize--;
                        if(pre_lruNode->next == NULL){
                            // 迁移tail指针
                            ssd->Lrutail = pre_lruNode;
                        }
                        break;
                    }
                    pre_lruNode=pre_lruNode->next;
                }
            }
            ssd->Lrutail->next = lru_node;
            ssd->Lrutail = lru_node;
            ssd->Lrusize++;
            return TRUE;
        }else if(ssd->Lrucapacity <= ssd->Lrusize){
            int is_rep = 0;
            struct CacheNode *pre_lruNode = ssd->Lruhead;
            if(ssd->Lruhead->superblockID == lru_node->superblockID){   // 头节点就是
                ssd->Lruhead = ssd->Lruhead->next;
                is_rep = 1;
            }else{  // 头节点不是，跳过头处理
                while(pre_lruNode->next != NULL){
                    if(pre_lruNode->next->superblockID == lru_node->superblockID){    // 有重复的SBblkID
                        // 当前的下一个重复了，删除下一个
                        pre_lruNode->next = pre_lruNode->next->next;
                        is_rep = 1;
                        if(pre_lruNode->next == NULL){
                            // 迁移tail指针
                            ssd->Lrutail = pre_lruNode;
                        }
                        break;
                    }
                    pre_lruNode=pre_lruNode->next;
                }
            }
            //lru满了，先头出再尾插
            if(is_rep == 0){    //且没有重复
                struct CacheNode *temp = ssd->Lruhead;
                ssd->Lruhead = temp->next;
                free(temp);
            }
            ssd->Lrutail->next = lru_node;
            ssd->Lrutail = lru_node;
            return TRUE;
        }else{  //都没有进入循环，返回失败
            return FALSE;
        }
}


unsigned int get_ppn_for_gc_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    unsigned int ppn;
    unsigned int active_block,block,page;
    int i;

    unsigned int available_page;
#ifdef DEBUG
    printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
    /*
    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
    {
        printf("\n\n Error int get_ppn_for_gc().\n");
        return 0xffffffff;
    }
    */
    //available_page = find_active_block_lf(ssd,channel,chip,die,plane);


    //找空闲块，再找活跃块，找到后修改相关参数
    available_page = find_active_block_msb(ssd, channel, chip, die, plane);
    if(available_page==FAILURE){
        available_page = find_active_block_lf(ssd,channel,chip,die,plane);
    }

    if(available_page == SUCCESS_LSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block;
        //printf("the last write lsb page: %d.\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb);
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
        ssd->free_lsb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb;
        ssd->write_lsb_count++;
        ssd->newest_write_lsb_count++;
    }
    else if(available_page == SUCCESS_CSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_csb_block;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb;
        ssd->write_csb_count++;
        ssd->free_csb_count--;
        ssd->newest_write_csb_count++;
    }
    else if(available_page == SUCCESS_MSB){
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_msb_block;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb += 3;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb;
        ssd->write_msb_count++;
        ssd->free_msb_count--;
        ssd->newest_write_msb_count++;
    }
    else{
        printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
        while(1){
        }
    }


    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=ssd->parameter->page_block){
        printf("ERROR! the last write page larger than the number of pages per block!! pos 2\n");
        for(i=0;i<ssd->parameter->block_plane;i++){
            printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_lsb);
        }
        printf("\n");
        for(i=0;i<ssd->parameter->block_plane;i++){
            printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_page);
        }
        printf("\n");
        //printf("The page number: %d.\n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
        while(1){}
    }


    //active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

    //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;
    //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
    /*
    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>ssd->parameter->page_block)
    {
        printf("DDD error! the last write page larger than 64!!\n");
        while(1){}
    }
    */

    //根据找到的活跃块去找相应的ppn并修改参数模拟写入数据，返回相应的ppn
    block=active_block;
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;

    ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

    ssd->program_count++;
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    ssd->write_flash_count++;

    return ppn;
}


unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    if(ssd->parameter->turbo_mode != 0){
        return get_ppn_for_gc_lf(ssd, channel, chip, die, plane);
    }
    unsigned int ppn;
    unsigned int active_block,block,page;

#ifdef DEBUG
    printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

    //直接去找活跃块
    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
    {
        printf("\n\n Error int get_ppn_for_gc().\n");
        return 0xffffffff;
    }

    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

    //*********************************************
    /*
    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb+3;
        }
    else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb+3;
        }
    else{
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb+3;
        }
        */
    //*********************************************

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>ssd->parameter->page_block)
    {
        printf("DDD error! the last write page larger than 64!!\n");
        while(1){}
    }

    block=active_block;
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;

    if(page%3==0){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb=page;
        ssd->free_lsb_count--;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
        //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->write_lsb_count++;
        ssd->newest_write_lsb_count++;
    }
    else if(page%3==2){
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb=page;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
        //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
        ssd->write_msb_count++;
        ssd->free_msb_count--;
        ssd->newest_write_msb_count++;
    }
    else{
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb=page;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
        ssd->write_csb_count++;
        ssd->free_csb_count--;
        ssd->newest_write_csb_count++;
    }

    ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

    ssd->program_count++;
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    ssd->write_flash_count++;

    return ppn;

}

/*********************************************************************************************************************
* 朱志明 于2011年7月28日修改
*函数的功能就是erase_operation擦除操作，把channel，chip，die，plane下的block擦除掉
*也就是初始化这个block的相关参数，eg：free_page_num=page_block，invalid_page_num=0，last_write_page=-1，erase_count++
*还有这个block下面的每个page的相关参数也要修改。
*********************************************************************************************************************/
Status erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die ,unsigned int plane ,unsigned int block)
{
    unsigned int flag, i;
    flag = 0;
    //遍历要擦除块的页，若块中有为有效页的，则将flag置一并报错
    for(i=0;i<ssd->parameter->page_block;i++){
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state!=0){
            flag = 1;
        }
    }
    if(flag==1){
        printf("Erasing a block with valid data: %d, %d, %d, %d, %d.\n",channel, chip, die, plane, block);
        return FAILURE;
    }

    //获取该块的当前页数量
    unsigned int origin_free_page_num, origin_free_lsb_num, origin_free_csb_num, origin_free_msb_num;
    origin_free_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num;
    origin_free_lsb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num;
    origin_free_csb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_csb_num;
    origin_free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num;
    // if(origin_free_lsb_num+origin_free_csb_num+origin_free_msb_num!=origin_free_page_num){
    // 	printf("WRONG METADATA in BLOCK LEVEL.(erase_operation)\n");
    // 	}
    //printf("ERASE_OPERATION: %d, %d, %d, %d, %d.\n",channel,chip,die,plane,block);
    //unsigned int i=0;

    //清空该块（就是将该块的参数修改为初始参数）
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num=ssd->parameter->page_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num=0;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].super_flag = 0;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=-1;
    //*****************************************************
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_lsb=-3;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_csb=-2;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_msb=-1;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num=(int)(ssd->parameter->page_block/3);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_csb_num=(int)(ssd->parameter->page_block/3);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num=(int)(ssd->parameter->page_block/3);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_lsb_num=0;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase=FALSE;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state=DR_STATE_NULL;
    //*****************************************************

    //擦除次数加1，同时遍历该块的所有页，将页的参数清除
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;
    // printf("%d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count);
    for (i=0;i<ssd->parameter->page_block;i++)
    {
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn=-1;
    }
    //全局以及各级别的擦除次数统计
    ssd->erase_count++;
    ssd->channel_head[channel].erase_count++;
    ssd->channel_head[channel].chip_head[chip].erase_count++;
    ssd->superblock[block+(chip * ssd->parameter->block_plane)].superblock_erase++;
    //加上再减相当于加上的无效页的数量，也就是增加了计数
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page+=ssd->parameter->page_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page-=origin_free_page_num;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num+=(ssd->parameter->page_block/3);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num-=origin_free_lsb_num;
    ssd->free_lsb_count+=(ssd->parameter->page_block/3);
    ssd->free_csb_count+=(ssd->parameter->page_block/3);
    ssd->free_msb_count+=(ssd->parameter->page_block/3);
    ssd->free_lsb_count-=origin_free_lsb_num;
    ssd->free_csb_count-=origin_free_csb_num;
    ssd->free_msb_count-=origin_free_msb_num;

    //添加擦除时间

    return SUCCESS;
}


/**************************************************************************************
*这个函数的功能是处理INTERLEAVE_TWO_PLANE，INTERLEAVE，TWO_PLANE，NORMAL下的擦除的操作。
***************************************************************************************/
Status erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
    unsigned int die=0;
    unsigned int plane=0;
    unsigned int block=0;
    struct direct_erase *direct_erase_node=NULL;
    unsigned int block0=0xffffffff;
    unsigned int block1=0;

    if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node==NULL)||
       ((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))     /*如果没有擦除操作，或者command不对，返回错误*/
    {
        return ERROR;
    }

    /************************************************************************************************************
    *处理擦除操作时，首先要传送擦除命令，这时channel，chip处于传送命令的状态，即CHANNEL_TRANSFER，CHIP_ERASE_BUSY
    *下一状态是CHANNEL_IDLE，CHIP_IDLE。
    *************************************************************************************************************/
    block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;

    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;


    //同时擦除两个平面中的相同块
    if(command==INTERLEAVE_TWO_PLANE)                                       /*高级命令INTERLEAVE_TWO_PLANE的处理*/
    {
        for(die=0;die<ssd->parameter->die_chip;die++)
        {
            block0=0xffffffff;
            if(die==die1)
            {
                block0=block1;
            }
            for (plane=0;plane<ssd->parameter->plane_die;plane++)
            {
                direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
                if(direct_erase_node!=NULL)
                {

                    block=direct_erase_node->block;

                    if(block0==0xffffffff)
                    {
                        block0=block;
                    }
                    else
                    {
                        if(block!=block0)
                        {
                            continue;
                        }

                    }
                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die,plane,block);     /*真实的擦除操作的处理*/
                    free(direct_erase_node);
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                }

            }
        }

        ssd->interleave_mplane_erase_count++;                             /*发送了一个interleave two plane erase命令,并计算这个处理的时间，以及下一个状态的时间*/
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+18*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tWB;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time-9*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tBERS;

    }

        //轮流选择平面的方式并行擦除不同Die上的块
    else if(command==INTERLEAVE)                                          /*高级命令INTERLEAVE的处理*/
    {
        for(die=0;die<ssd->parameter->die_chip;die++)
        {
            for (plane=0;plane<ssd->parameter->plane_die;plane++)
            {
                if(die==die1)
                {
                    plane=plane1;
                }
                direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
                if(direct_erase_node!=NULL)
                {
                    block=direct_erase_node->block;
                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die,plane,block);
                    free(direct_erase_node);
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                    break;
                }
            }
        }
        ssd->interleave_erase_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
    }

        //选择同一die中不同plane但相同块号的块进行擦除
    else if(command==TWO_PLANE)                                          /*高级命令TWO_PLANE的处理*/
    {

        for(plane=0;plane<ssd->parameter->plane_die;plane++)
        {
            direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node;
            if((direct_erase_node!=NULL))
            {
                block=direct_erase_node->block;
                if(block==block1)
                {
                    ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die1,plane,block);
                    free(direct_erase_node);
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                }
            }
        }

        ssd->mplane_erase_conut++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
    }

        //直接找到输入对应的擦除块然后擦除
    else if(command==NORMAL)                                             /*普通命令NORMAL的处理*/
    {
        direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
        block=direct_erase_node->block;
        ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node=direct_erase_node->next_node;
        free(direct_erase_node);
        direct_erase_node=NULL;
        erase_operation(ssd,channel,chip,die1,plane1,block);

        ssd->direct_erase_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ssd->parameter->time_characteristics.tBERS;
    }
    else
    {
        return ERROR;
    }

    //检查是否还有擦除节点，有就报错
    direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

    if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
    {
        return FAILURE;
    }
    else
    {
        return SUCCESS;
    }
}


//快速擦除，即进行一个条件判断然后直接使用normal模式
Status fast_erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
    unsigned int die=0;
    unsigned int plane=0;
    unsigned int block=0;
    struct direct_erase *direct_erase_node=NULL;
    unsigned int block0=0xffffffff;
    unsigned int block1=-1;
    /*
    if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node==NULL)|| (command!=NORMAL))
    {
        return ERROR;
    }
    */
    /************************************************************************************************************
    *处理擦除操作时，首先要传送擦除命令，这是channel，chip处于传送命令的状态，即CHANNEL_TRANSFER，CHIP_ERASE_BUSY
    *下一状态是CHANNEL_IDLE，CHIP_IDLE。
    检查目标plane1中的每个块，寻找符合特定条件的物理块block1，即满足以下条件：

    free_msb_num等于page_block/2（即半个物理块的LSB页可用）。
    invalid_lsb_num等于page_block/2（即半个物理块的MSB页失效）。
    fast_erase标志不为TRUE，即该物理块未被标记为快速擦除。

如果找到符合条件的block1，则将其标记为快速擦除（fast_erase设为TRUE）。
    *************************************************************************************************************/
    struct plane_info candidate_plane;
    candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1];
    while(block < ssd->parameter->block_plane)
    {
        if((candidate_plane.blk_head[block].free_msb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].invalid_lsb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].fast_erase!=TRUE))
        {
            block1 = block;
            break;
        }
        block++;
    }
    //block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node->block;
    if(block1==-1){
        return FAILURE;
    }
    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;


    if(command==NORMAL)                                             /*普通命令NORMAL的处理*/
    {
        //direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node;
        //block=direct_erase_node->block;
        //ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node=direct_erase_node->next_node;
        //free(direct_erase_node);
        //direct_erase_node=NULL;
        candidate_plane.blk_head[block1].fast_erase = TRUE;
        erase_operation(ssd,channel,chip,die1,plane1,block1);

        ssd->fast_gc_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ssd->parameter->time_characteristics.tBERS;
        return SUCCESS;
    }
    else
    {
        printf("Wrong Commend for fast gc.\n");
        return ERROR;
    }
    /*
    direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node;

    if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
    {
        return FAILURE;
    }
    else
    {
        return SUCCESS;
    }
    */
}


/*******************************************************************************************************************
*GC操作由某个plane的free块少于阈值进行触发，当某个plane被触发时，GC操作占据这个plane所在的die，因为die是一个独立单元。
*对一个die的GC操作，尽量做到四个plane同时erase，利用interleave erase操作。GC操作应该做到可以随时停止（移动数据和擦除
*时不行，但是间隙时间可以停止GC操作），以服务新到达的请求，当请求服务完后，利用请求间隙时间，继续GC操作。可以设置两个
*GC阈值，一个软阈值，一个硬阈值。软阈值表示到达该阈值后，可以开始主动的GC操作，利用间歇时间，GC可以被新到的请求中断；
*当到达硬阈值后，强制性执行GC操作，且此GC操作不能被中断，直到回到硬阈值以上。
*在这个函数里面，找出这个die所有的plane中，有没有可以直接删除的block，要是有的话，利用interleave two plane命令，删除
*这些block，否则有多少plane有这种直接删除的block就同时删除，不行的话，最差就是单独这个plane进行删除，连这也不满足的话，
*直接跳出，到gc_parallelism函数进行进一步GC操作。该函数寻找全部为invalid的块，直接删除，找到可直接删除的返回1，没有找
*到返回-1。
*********************************************************************************************************************/
int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    unsigned int lv_die=0,lv_plane=0; /*为避免重名而使用的局部变量 Local variables*/
    unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
    unsigned int normal_erase_flag=TRUE;

    struct direct_erase * direct_erase_node1=NULL;
    struct direct_erase * direct_erase_node2=NULL;

    direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;//单位：block
    if (direct_erase_node1==NULL)
    {
        return FAILURE;
    }

    /********************************************************************************************************
    *当能处理TWOPLANE高级命令时，就在相应的channel，chip，die中两个不同的plane找到可以执行TWOPLANE操作的block
    *并置muilt_plane_flag为TRUE
    *********************************************************************************************************/
    if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
    {
        for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
        {
            direct_erase_node2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;     //这里的plane应该改成lv_plane吧？？？
            if((lv_plane!=plane)&&(direct_erase_node2!=NULL))
            {
                if((direct_erase_node1->block)==(direct_erase_node2->block))
                {
                    muilt_plane_flag=TRUE;
                    break;
                }
            }
        }
    }

    /***************************************************************************************
    *当能处理INTERLEAVE高级命令时，就在相应的channel，chip找到可以执行INTERLEAVE的两个block
    *并置interleaver_flag为TRUE
    ****************************************************************************************/
    if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
    {
        for(lv_die=0;lv_die<ssd->parameter->die_chip;lv_die++)
        {
            if(lv_die!=die)
            {
                for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
                {
                    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node!=NULL)      //这个遍历也没用
                    {
                        interleaver_flag=TRUE;
                        break;
                    }
                }
            }
            if(interleaver_flag==TRUE)
            {
                break;
            }
        }
    }

    /************************************************************************************************************************
    *A，如果既可以执行twoplane的两个block又有可以执行interleaver的两个block，那么就执行INTERLEAVE_TWO_PLANE的高级命令擦除操作
    *B，如果只有能执行interleaver的两个block，那么就执行INTERLEAVE高级命令的擦除操作
    *C，如果只有能执行TWO_PLANE的两个block，那么就执行TWO_PLANE高级命令的擦除操作
    *D，没有上述这些情况，那么就只能够执行普通的擦除操作了
    *************************************************************************************************************************/
    if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    }

    if ((normal_erase_flag==TRUE))                              /*不是每个plane都有可以直接删除的block，只对当前plane进行普通的erase操作，或者只能执行普通命令*/
    {
        if (erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)
        {
            return SUCCESS;
        }
        else
        {
            return FAILURE;                                     /*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作*/
        }
    }
    return SUCCESS;
}


//只执行normal模式
int gc_direct_fast_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    unsigned int lv_die=0,lv_plane=0;                                                           /*为避免重名而使用的局部变量 Local variables*/
    unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
    unsigned int normal_erase_flag=TRUE;

    struct direct_erase * direct_erase_node1=NULL;
    struct direct_erase * direct_erase_node2=NULL;

    /*
    direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].fast_erase_node;
    if (direct_erase_node1==NULL)
    {
        return FAILURE;
    }
    */
    /********************************************************************************************************
    *当能处理TWOPLANE高级命令时，就在相应的channel，chip，die中两个不同的plane找到可以执行TWOPLANE操作的block
    *并置muilt_plane_flag为TRUE
    *********************************************************************************************************/
    if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
    {
        printf("Advanced commands are NOT supported.\n");
        return FAILURE;
    }

    /***************************************************************************************
    *当能处理INTERLEAVE高级命令时，就在相应的channel，chip找到可以执行INTERLEAVE的两个block
    *并置interleaver_flag为TRUE
    ****************************************************************************************/
    if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
    {
        printf("Advanced commands are NOT supported.\n");
        return FAILURE;
    }

    /************************************************************************************************************************
    *A，如果既可以执行twoplane的两个block又有可以执行interleaver的两个block，那么就执行INTERLEAVE_TWO_PLANE的高级命令擦除操作
    *B，如果只有能执行interleaver的两个block，那么就执行INTERLEAVE高级命令的擦除操作
    *C，如果只有能执行TWO_PLANE的两个block，那么就执行TWO_PLANE高级命令的擦除操作
    *D，没有上述这些情况，那么就只能够执行普通的擦除操作了
    *************************************************************************************************************************/
    /*
    if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    */

    if ((normal_erase_flag==TRUE))                              /*不是每个plane都有可以直接删除的block，只对当前plane进行普通的erase操作，或者只能执行普通命令*/
    {
        if (fast_erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)
        {
            return SUCCESS;
        }
        else
        {
            return FAILURE;                                     /*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作*/
        }
    }
    else{
        printf("Wrong command for fast gc.\n");
        return FAILURE;
    }
    //return SUCCESS;
}


Status move_page(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size)
{
    struct local *new_location=NULL;
    struct sub_request* sub=NULL;
    unsigned int free_state=0,valid_state=0;
    unsigned int lpn=0,old_ppn=0,ppn=0;

    lpn=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
    valid_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
    free_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
    old_ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);      /*记录这个有效移动页的ppn，对比map或者额外映射关系中的ppn，进行删除和添加操作*/
    // old_location = find_location(ssd,old_ppn);
    ssd->pagemove_write++;


    sub = (struct sub_request*)malloc(sizeof(struct sub_request));     /*申请一个子请求sub的空间*/
    alloc_assert(sub,"sub_request");
    memset(sub,0, sizeof(struct sub_request));

    if(sub==NULL)//分配失败的情况
    {
        return NULL;
    }

    //若分配成功，初始化子请求sub的成员变量值
    sub->location=NULL;
    sub->next_node=NULL;
    sub->next_subs=NULL;
    sub->update=NULL;

    sub->ppn=0;
    sub->operation = WRITE;
    sub->location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(sub->location,"sub->location");
    memset(sub->location,0, sizeof(struct local));

    sub->current_state=SR_WAIT;
    sub->current_time=ssd->current_time;
    sub->lpn=lpn;
    sub->size=size(valid_state);
    sub->state=valid_state;

    sub->begin_time=ssd->current_time;
    ssd->dram->map->map_entry[lpn].pn = 0;
    ssd->dram->map->map_entry[lpn].state = 0;
    //调用allocate_location()函数为sub分配物理地址
    //若非ERROR则创建写子请求成功直接返回sub
    //若allocate_location()返回ERROR则说明分配失败此时需要释放malloc动态申请到的location空间，且释放sub和返回NULL表示创建写子请求失败。
    if (allocate_location_gc(ssd ,sub)==ERROR)
    {
        free(sub->location);
        sub->location=NULL;
        free(sub);
        sub=NULL;
        return NULL;
    }
    // location->channel=ssd->token;   //记录着当前已经分配出去供程序使用的物理channel通道号
    // ssd->token=(ssd->token+1)%ssd->parameter->channel_number;  //若使用则立即更新

    // location->chip=ssd->channel_head[location->channel].token; //继续往下层级申请到plane
    // ssd->channel_head[location->channel].token=(location->chip+1)%ssd->parameter->chip_channel[0];

    // location->die=ssd->channel_head[location->channel].chip_head[location->chip].token;
    // ssd->channel_head[location->channel].chip_head[location->chip].token=(location->die+1)%ssd->parameter->die_chip;

    // location->plane=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].token;
    // ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].token=(location->plane+1)%ssd->parameter->plane_die;

    // ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane);           /*找出来的ppn一定是在发生gc操作的plane中,才能使用copyback操作，为gc操作获取ppn*/

    // new_location=find_location(ssd,ppn);          /*根据新获得的ppn获取new_location*/
    //printf("MOVE PAGE, lpn:%d, old_ppn:%d, new_ppn:%d, FROM %d,%d,%d,%d,%d,%d TO %d,%d,%d,%d,%d,%d.\n",lpn,old_ppn,ppn,location->channel,location->chip,location->die,location->plane,location->block,location->page,new_location->channel,new_location->chip,new_location->die,new_location->plane,new_location->block,new_location->page);
    /*
    if(new_location->channel==location->channel && new_location->chip==location->chip && new_location->die==location->die && new_location->plane==location->plane && new_location->block==location->block){
        printf("MOVE PAGE WRONG!!! Page is moved to the same block.\n");
        return FAILURE;
        }
    */
    // if(new_location->block == location->block)
    // 	{
    // 		printf("Data is moved to the same block!!!!!!\n");
    // 		return FAILURE;
    // 	}
    // if(new_location->channel == old_location->channel && new_location->chip == old_location->chip && new_location->die == old_location->die && new_location->plane == old_location->plane){
    // 	printf("Data is moved to the same block!!!!!!\n");
    // 	return FAILURE;
    // 	}
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)  //
    {
        printf("Wrong COMMANDS.\n");
        return FAILURE;

        //后面的应该不会执行吧？那就先不看
        if (ssd->parameter->greed_CB_ad==1)                                                                /*贪婪地使用高级命令*/
        {
            ssd->copy_back_count++;
            ssd->gc_copy_back++;
            while (old_ppn%2!=ppn%2)
            {
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;

                free(new_location);
                new_location=NULL;

                ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane);    /*找出来的ppn一定是在发生gc操作的plane中，并且满足奇偶地址限制,才能使用copyback操作*/
                ssd->program_count--;
                ssd->write_flash_count--;
                ssd->waste_page_count++;
            }
            if(new_location==NULL)
            {
                new_location=find_location(ssd,ppn);
            }

            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;
        }
        else
        {
            if (old_ppn%2!=ppn%2)
            {
                (* transfer_size)+=size(valid_state);
            }
            else
            {
                ssd->copy_back_count++;
                ssd->gc_copy_back++;
            }
        }
    }
    else
    {
        //计算所要移动的大小
        (* transfer_size)+=size(valid_state);
        // req_current_size = size(valid_state);
    }

    //把旧页的状态，对应的lpn等复制到新页
    // ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
    // ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
    // ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;

    //将旧页设为无效页
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
    // if((location->page)%3==0){   //表示该页为lsb
    // 	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
    // 	}

    // if (old_ppn==ssd->dram->map->map_entry[lpn].pn)            /*修改映射表*/
    // {
    // 	ssd->dram->map->map_entry[lpn].pn=ppn;
    // }
    ssd->moved_page_count++;

    free(new_location);
    new_location=NULL;
    // free(old_location);
    // old_location=NULL;

    return SUCCESS;
}

/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在不能中断的gc操作(直接gc）中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的,擦除invalid_page_num最多的block
********************************************************************************************************************************************/
int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    unsigned int i=0,invalid_page=0;
    unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*记录失效页最多的块号*/
    struct local *  location=NULL;
    unsigned int total_invalid_page_num=0;

    if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)                                           /*获取活跃块*/
    {
        printf("\n\n Error in uninterrupt_gc().\n");
        return ERROR;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

    invalid_page=0;
    transfer_size=0;
    block=-1;
    for(i=0;i<ssd->parameter->block_plane;i++)    /*查找最多invalid_page的块号，以及最大的invalid_page_num*/
    {
        total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].free_page_num > 0){  //有空闲页就跳过这个块
            continue;
        }
        if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))
        {
            invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
            block=i;
        }
    }
    if (block==-1)
    {
        printf("123\n");
        // while(1){};
        return 1;
    }

    //if(invalid_page<5)
    //{
    //printf("\ntoo less invalid page. \t %d\t %d\t%d\t%d\t%d\t%d\t\n",invalid_page,channel,chip,die,plane,block);
    //}
    //***********************************************
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
    //***********************************************
    free_page=0;
    for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
        {
            free_page++;
        }
        if(free_page!=0)
        {
            //printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
        }
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));

            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            move_page(ssd, location, &transfer_size);                                         /*真实的move_page操作*/
            page_move_count++;

            free(location);
            location=NULL;
        }
    }
    erase_operation(ssd,channel ,chip , die,plane ,block);	                                              /*执行完move_page操作后，就立即执行block的擦除操作*/

    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        }
    }
    else
    {

        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

    }

    return 1;
}

int uninterrupt_gc_super(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int blk_id)
{
    unsigned int free_page,i,block;
    unsigned int page_move_count = 0,transfer_size = 0;
    struct local * location = NULL;
    //***********************************************
    block = blk_id;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
    // //***********************************************
    free_page=0;
    // unsigned int m = 0;
    for(i=0;i<ssd->parameter->page_block;i++)/*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
        {
            free_page++;
        }
        if(free_page!=0)
        {
            //printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
        }
        //超级块的有效页迁移
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));
            // valid_state = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state;
            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            move_page(ssd, location, &transfer_size);  /*真实的move_page操作*/
            page_move_count++;
            // m = location->channel;
            // ssd->channel_head[m].move_count++;
            // ssd->channel_head[m].req_transfer_size += size(valid_state);
            free(location);
            location=NULL;
        }
    }
    erase_operation(ssd, channel, chip, die, plane, block);	    /*执行完move_page操作后，就立即执行block的擦除操作*/
    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS; // 擦除时间去掉
        }
    }
    else
    {
        // ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        // ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR)+transfer_size*SECTOR*ssd->parameter->time_characteristics.tRC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
    }
    return 1;
}

int uninterrupt_gc_super_soft(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int blk_id)
{
    unsigned int free_page,i,block;
    unsigned int page_move_count = 0,transfer_size = 0;
    struct local * location = NULL;
    //***********************************************
    block = blk_id;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
    // //***********************************************
    free_page=0;
    // unsigned int m = 0;
    for(i=0;i<ssd->parameter->page_block;i++)/*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
        {
            free_page++;
        }
        if(free_page!=0)
        {
            //printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
        }
        //超级块的有效页迁移
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));
            // valid_state = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state;
            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            move_page(ssd, location, &transfer_size);  /*真实的move_page操作*/
            page_move_count++;
            // m = location->channel;
            // ssd->channel_head[m].move_count++;
            // ssd->channel_head[m].req_transfer_size += size(valid_state);
            free(location);
            location=NULL;
        }
    }
    // page_move执行完之后不立即进行擦除
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = FALSE;
    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    // ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            // ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS; // 擦除时间去掉
        }
    }
    else
    {
        // ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        // ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR)+transfer_size*SECTOR*ssd->parameter->time_characteristics.tRC;
        // ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
    }
    return 1;
}


// 首先，根据优先级和设置的阈值，选择一个失效的块（block1）作为垃圾回收的目标。如果找不到满足条件的块，则直接返回成功，不执行垃圾回收。
// 根据优先级判断是否满足执行垃圾回收的条件，如果不满足，也直接返回成功，不执行垃圾回收。
// 对选定的目标块（block1）进行垃圾回收操作，将其中的有效页移动到其他块中，并进行擦除操作。
// 更新相关状态信息，如通道状态、芯片状态、垃圾回收计数等。
// 计算通道和芯片的下一个状态预测时间，以便进入下一个状态。
int uninterrupt_fast_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int priority)
{
    //printf("===>Enter uninterrupt_fast_gc.\n");
    unsigned int i=0,invalid_page=0;
    unsigned int block,block1,active_block,transfer_size,free_page,page_move_count=0;                           /*记录失效页最多的块号*/
    struct local *  location=NULL;
    unsigned int total_invalid_page_num=0;

    block1 = -1;
    block = 0;
    struct plane_info candidate_plane;
    candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
    unsigned int invalid_lsb_num = 0;

    //找到无效低页数最多的块
    while(block < ssd->parameter->block_plane-1){
        //printf("Enter while, %d\n", block);
        if((candidate_plane.blk_head[block].free_msb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].fast_erase!=TRUE)){
            if(candidate_plane.blk_head[block].free_lsb_num==0 && candidate_plane.blk_head[block].invalid_lsb_num>invalid_lsb_num){
                block1 = block;
                invalid_lsb_num = candidate_plane.blk_head[block].invalid_lsb_num;
            }
        }
        block++;
    }
    if (block1==-1)
    {
        //printf("====>No block has invalid LSB page??\n");
        return SUCCESS;
    }

    //根据priority命令的不同判断是否满足gc条件
    if(priority==GC_FAST_UNINTERRUPT_EMERGENCY){
        if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_2){
            return SUCCESS;
        }
    }
    else if(priority==GC_FAST_UNINTERRUPT){
        if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_1){
            return SUCCESS;
        }
    }
    else if(priority==GC_FAST_UNINTERRUPT_IDLE){
        if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_idle){
            return SUCCESS;
        }
        //printf("FAST_GC_IDLE on %d, %d, %d, %d, %d.\n", channel, chip, die, plane, block1);
        ssd->idle_fast_gc_count++;
    }
    else{
        printf("GC_FAST ERROR. PARAMETERS WRONG.\n");
        return SUCCESS;
    }

    free_page=0;
    struct blk_info candidate_block;
    candidate_block = candidate_plane.blk_head[block1];
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block1].fast_erase = TRUE;
    //printf("%d %d %d %d %d is selected, free_msbs:%d, invalid_lsbs:%d.\n",channel,chip,die,plane,block1, candidate_block.free_msb_num, candidate_block.invalid_lsb_num);
    //printf("begin to move pages.\n");
    //printf("<<<Fast GC Candidate, %d, %d, %d, %d, %d.\n",channel,chip,die,plane,block1);
    //printf("Begin to MOVE PAGES.\n");
    for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if(candidate_block.page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            //printf("move page %d.\n",i);
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));

            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block1;
            location->page=i;
            //printf("state of this block: %d.\n",candidate_block.fast_erase);
            move_page(ssd, location, &transfer_size);                                                   /*真实的move_page操作*/
            page_move_count++;

            free(location);
            location=NULL;
        }
    }
    //printf("block: %d, %d, %d, %d, %d going to be erased.\n",channel, chip, die, plane, block1);
    erase_operation(ssd,channel ,chip , die, plane ,block1);	                                              /*执行完move_page操作后，就立即执行block的擦除操作*/
    //printf("ERASE OPT: %d, %d, %d, %d, %d\n",channel,chip,die,plane,block1);
    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

    ssd->fast_gc_count++;

    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        }
    }
    else
    {

        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

    }
    //printf("===>Exit uninterrupt_fast_gc Successfully.\n");
    return 1;
}


//感觉跟gc差不多，都是先去找失效页最多的块，然后把有效页移走，再把该块清空，只是不用去更改相关的状态
int uninterrupt_dr(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
    unsigned int i=0,invalid_page=0;
    unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*记录失效页最多的块号*/
    struct local *  location=NULL;
    unsigned int total_invalid_page_num=0;
    /*
    if(find_active_block_dr(ssd,channel,chip,die,plane)!=SUCCESS)
    {
        printf("\n\n Error in uninterrupt_dr(). No active block\n");
        return ERROR;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    */
    invalid_page=0;
    transfer_size=0;
    block=-1;
    for(i=0;i<ssd->parameter->block_plane;i++)                                                           /*查找最多invalid_page的块号，以及最大的invalid_page_num*/
    {
        total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page)
        {
            invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
            block=i;
        }
    }

    if (block==-1)
    {
        printf("No block is selected.\n");
        return 1;
    }
    //把目标block的dr_state设为输出
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state=DR_STATE_OUTPUT;


    //printf("Block %d with %d invalid pages is selected.\n", block, ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num);
    free_page=0;
    for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
        {
            free_page++;
        }
        /*
        if(free_page!=0)
        {
            //printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
        }
        */
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));

            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state != DR_STATE_OUTPUT){
                printf("The DR_STATE is not settle.\n");
            }
            move_page(ssd, location, &transfer_size);                                                   /*真实的move_page操作*/
            page_move_count++;

            free(location);
            location=NULL;
        }
    }
    erase_operation(ssd,channel ,chip , die,plane ,block);	                                              /*执行完move_page操作后，就立即执行block的擦除操作*/
    /*
    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
    */
    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    /*
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        }
    }
    else
    {

        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

    }
    */
    return 1;
}


/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在可以中断的gc操作中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的
********************************************************************************************************************************************/
int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)
{
    unsigned int free_page,i,block;
    unsigned int page_move_count = 0,transfer_size = 0;
    struct local * location = NULL;
    //***********************************************
    block = gc_node->block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
    // //***********************************************
    free_page=0;
    // unsigned int m = 0;
    for(i=0;i<ssd->parameter->page_block;i++)/*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
    {
        if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
        {
            free_page++;
        }
        if(free_page!=0)
        {
            //printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
        }
        //超级块的有效页迁移
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*该页是有效页，需要copyback操作*/
        {
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));
            // valid_state = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state;
            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            move_page(ssd, location, &transfer_size);  /*真实的move_page操作*/
            page_move_count++;
            // m = location->channel;
            // ssd->channel_head[m].move_count++;
            // ssd->channel_head[m].req_transfer_size += size(valid_state);
            free(location);
            location=NULL;
        }
    }
    erase_operation(ssd,channel ,chip , die,plane ,block);	    /*执行完move_page操作后，就立即执行block的擦除操作*/
    ssd->channel_head[channel].current_state=CHANNEL_GC;
    ssd->channel_head[channel].current_time=ssd->current_time;
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
    /***************************************************************
    *在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
    *channel下个状态时间的计算，以及chip下个状态时间的计算
    ***************************************************************/
    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {

            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        }
    }
    else
    {
        // ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
        // ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count* (7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR)+transfer_size*SECTOR*ssd->parameter->time_characteristics.tRC;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
    }
    return 1;
}

/*************************************************************
*函数的功能是当处理完一个gc操作时，需要把gc链上的gc_node删除掉
**************************************************************/
int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node)
{
    int flag = 1;
    struct gc_operation *gc_pre=NULL;
    if(gc_node==NULL)
    {
        return ERROR;
    }
    // 如果是channel gc队列的头结点
    if (gc_node==ssd->channel_head[channel].gc_command && ssd->channel_head[channel].gc_command->next_node != NULL)
    {
        ssd->channel_head[channel].gc_command = gc_node->next_node;
        flag = 0;
        // ssd->channel_head[channel].gc_command_tail = gc_node->next_node;
    }else if(ssd->channel_head[channel].gc_command->next_node == NULL){ // 最后一个节点的删除
        ssd->channel_head[channel].gc_command = NULL;
        ssd->channel_head[channel].gc_command_tail = NULL;
        flag = 0;
    }else{
        gc_pre=ssd->channel_head[channel].gc_command;
        while (gc_pre->next_node!=NULL)
        {
            if (gc_pre->next_node==gc_node)
            {
                flag = 0;
                gc_pre->next_node=gc_node->next_node;
                if(gc_node->next_node == NULL){
                    //说明删除的是尾节点
                    ssd->channel_head[channel].gc_command_tail = gc_pre;
                }
                break;
            }
            gc_pre=gc_pre->next_node;
        }
    }
    if(flag == 1){
        return FAILURE;
    }
    free(gc_node);
    gc_node=NULL;
    ssd->gc_request--;
    return SUCCESS;
}

/***************************************
*这个函数的功能是处理channel的每个gc操作(这里都是不可中断的gc)
****************************************/
Status gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
    int flag_direct_erase=1, flag_gc=1,flag_invoke_gc=1;
    unsigned int chip,die,plane,block,flag_priority=0;
    unsigned int current_state=0, next_state=0, type=0;
    long long next_state_predict_time=0;
    struct gc_operation *gc_node=NULL,*gc_p=NULL;

    /*******************************************************************************************
    *查找每一个gc_node，获取gc_node所在的chip的当前状态，下个状态，下个状态的预计时间
    *如果当前状态是空闲，或是下个状态是空闲而下个状态的预计时间小于当前时间，并且是不可中断的gc
    *那么就flag_priority令为1，否则为0
    ********************************************************************************************/
    gc_node=ssd->channel_head[channel].gc_command;
    while (gc_node!=NULL)
    {
        current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
        next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
        next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
        if((gc_node->type == 1)&&( (current_state==CHIP_IDLE) ||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time))))	// 是硬阈值而且满足宽松条件
        {
            if (gc_node->priority==GC_UNINTERRUPT)   /*这个gc请求是不可中断的，优先服务这个gc操作*/
            {
                flag_priority=1;
                break;
            }
        }else if((gc_node->type == 0)&&( (current_state==CHIP_IDLE) || ((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time))) && (ssd->max_queue_depth < 16)) // 是软阈值而且满足严苛条件
        {
            if (gc_node->priority==GC_UNINTERRUPT)   /*这个gc请求是不可中断的，优先服务这个gc操作*/
            {
                // ssd->cold_choose++;
                flag_priority=1;
                break;
            }
        }
        gc_node=gc_node->next_node;
    }
    if (flag_priority!=1)       /*没有找到不可中断的gc请求，首先执行队首的gc请求*/
    {
        gc_node=ssd->channel_head[channel].gc_command;
        while (gc_node!=NULL)
        {
            current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
            next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
            next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
            /**********************************************
            *需要gc操作的目标chip是空闲的，才可以进行gc操作
            ***********************************************/
            if((gc_node->type == 0) && ((current_state==CHIP_IDLE) || ((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time))) && (ssd->max_queue_depth < 16))  //
            {
                ssd->cold_choose++;
                break;
            }else if((gc_node->type == 1) && ((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time))))
            {
                break;
            }
            gc_node=gc_node->next_node;
        }

    }
    if(gc_node==NULL)
    {
        return FAILURE;
    }

    chip=gc_node->chip;
    die=gc_node->die;
    plane=gc_node->plane;
    block = gc_node->block;
    type = gc_node->type;

    if (gc_node->priority==GC_UNINTERRUPT)   //!!
    {
        // 如果是软阈值
        // if(type == 0){
        // 当channel很空闲
        // if((ssd->channel_head[channel].current_state==CHANNEL_IDLE)||(ssd->channel_head[channel].next_state == CHANNEL_IDLE && ssd->channel_head[channel].next_state_predict_time<=ssd->current_time)){
        // flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
        /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
        ssd->pagemove_block_count++;
        flag_gc=uninterrupt_gc_super_soft(ssd,channel,chip,die,plane,block); //不擦除
        // ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = FALSE;
        // 将小块的page_move标志位置为1，代表被page_move了
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 1;
        ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count++;
        // gc完成，对应SB的gc_count字段自增，但是如果count等于8，说明开启了新一轮gc
        if(ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count == 8){
            ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count = 0;
            ssd->superblock[block+(chip*ssd->parameter->block_plane)].is_softSB_inQue = 0;
            // 然后一起擦除
            for(int i=0; i<ssd->parameter->channel_number; i++){
                // ssd->channel_head[i].chip_head[chip].current_state=CHIP_ERASE_BUSY;
                ssd->channel_head[i].chip_head[chip].current_time=ssd->current_time;
                // ssd->channel_head[i].chip_head[chip].next_state=CHIP_IDLE;
                // ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
                ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 0;
                ssd->channel_head[i].chip_head[chip].next_state_predict_time = ssd->channel_head[i].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
                erase_operation(ssd, i, chip, die, plane, block);
                ssd->superblock[block+(chip*ssd->parameter->block_plane)].invalid_page_count = 0;
            }
        }

        if (flag_gc==1)
        {
            delete_gc_node(ssd,channel,gc_node);
        }

        return SUCCESS;
        // }else{
        //     return FAILURE;
        // }

        // }else{// 到达硬阈值，直接做

        //     /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
        //     flag_gc=uninterrupt_gc_super_soft(ssd,channel,chip,die,plane,block); //不擦除
        //     // ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = FALSE;
        //     // 将小块的page_move标志位置为1，代表被page_move了
        //     ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 1;
        //     ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count++;
        //     // gc完成，对应SB的gc_count字段自增，但是如果count等于8，说明开启了新一轮gc
        //     if(ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count == 8){
        //         ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count = 0;
        //         // 然后一起擦除
        //         for(int i=0; i<ssd->parameter->channel_number; i++){
        //             // ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
        //             ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 0;
        //             ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
        //             erase_operation(ssd, i, chip, die, plane, block);
        //         }
        //     }

        //     if (flag_gc==1)
        //     {
        //         delete_gc_node(ssd,channel,gc_node);
        //     }
        //     return SUCCESS;
        // }
    }

        /*******************************************************************************
        *可中断的gc请求，需要首先确认该channel上没有子请求在这个时刻需要使用这个channel，
        *没有的话，在执行gc操作，有的话，不执行gc操作
        ********************************************************************************/
    else if(gc_node->priority==GC_FAST_UNINTERRUPT || gc_node->priority==GC_FAST_UNINTERRUPT_EMERGENCY || gc_node->priority==GC_FAST_UNINTERRUPT_IDLE){
        //printf("===>GC_FAST on %d,%d,%d,%d begin.\n",channel,chip,die,plane);
        flag_direct_erase=gc_direct_fast_erase(ssd,channel,chip,die,plane);
        if (flag_direct_erase!=SUCCESS)
        {
            /*
            printf("Something Weird happened.\n");
            return FAILURE;
            */
            //printf("NO BLOCK CAN BE ERASED DIRECTLY.\n");
            flag_gc=uninterrupt_fast_gc(ssd,channel,chip,die,plane,gc_node->priority);
            if (flag_gc==1)
            {
                delete_gc_node(ssd,channel,gc_node);
            }
        }
        else
        {
            //printf("THERE IS BLOCK CAN BE ERASED DIRECTLY.\n");
            delete_gc_node(ssd,channel,gc_node);
        }
        //printf("===>GC_FAST on %d,%d,%d,%d successed.\n",channel,chip,die,plane);
        return SUCCESS;
    }
    else
    {
        flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
        if (flag_direct_erase!=SUCCESS)
        {
            /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
            flag_gc=interrupt_gc(ssd,channel,chip,die,plane,block); //不擦除
            // 将小块的page_move标志位置为1，代表被page_move了
            ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 1;
            ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count++;
            // gc完成，对应SB的gc_count字段自增，但是如果count等于8，说明开启了新一轮gc
            if(ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count == 8){
                ssd->superblock[block+(chip*ssd->parameter->block_plane)].gc_count = 0;
                // 然后一起擦除
                for(int i=0; i<ssd->parameter->channel_number; i++){
                    // ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
                    ssd->channel_head[i].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].SB_gc_flag = 0;
                    ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
                    erase_operation(ssd, i, chip, die, plane, block);
                }
            }

            if (flag_gc==1)
            {
                delete_gc_node(ssd,channel,gc_node);
            }
        }
        else
        {
            delete_gc_node(ssd,channel,gc_node);
        }
        return SUCCESS;
        // flag_invoke_gc=decide_gc_invoke(ssd,channel);                                  /*判断是否有子请求需要channel，如果有子请求需要这个channel，那么这个gc操作就被中断了*/

        // if (flag_invoke_gc==1)
        // {
        // 	flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
        // 	if (flag_direct_erase==-1)
        // 	{
        // 		flag_gc=interrupt_gc(ssd,channel,chip,die,plane,gc_node);             /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
        // 		if (flag_gc==1)
        // 		{
        // 			delete_gc_node(ssd,channel,gc_node);
        // 		}
        // 	}
        // 	else if (flag_direct_erase==1)
        // 	{
        // 		delete_gc_node(ssd,channel,gc_node);
        // 	}
        // 	return SUCCESS;
        // }
        // else
        // {
        // 	return FAILURE;
        // }
    }
}

//遍历这个plane上的每个块，只要块上无效页的数量大于dr的阈值，该plane中就有无效块
Status find_invalid_block(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane){
    unsigned int block, flag;
    for(block=0;block<ssd->parameter->block_plane;block++){
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num>(ssd->parameter->page_block*ssd->parameter->dr_filter)){
            return TRUE;
        }
    }
    return FALSE;
}

Status dr_for_channel(struct ssd_info *ssd, unsigned int channel)
{
    int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
    unsigned int chip,die,plane,flag_priority=0;
    unsigned int current_state=0, next_state=0;
    long long next_state_predict_time=0;
    struct gc_operation *gc_node=NULL,*gc_p=NULL;

    /*******************************************************************************************
    *查找每一个gc_node，获取gc_node所在的chip的当前状态，下个状态，下个状态的预计时间
    *如果当前状态是空闲，或是下个状态是空闲而下个状态的预计时间小于当前时间，并且是不可中断的gc
    *那么就flag_priority令为1，否则为0
    ********************************************************************************************/
    //======================================================================
    //所有垃圾回收操作都清除掉
    ssd->channel_head[channel].gc_command=NULL;
    gc_node=ssd->channel_head[channel].gc_command;
    //轮询所有plane，把有无效页的block都回收掉
    unsigned int plane_flag;
    unsigned int counter;
    for(chip=0;chip<ssd->parameter->chip_channel[0];chip++){
        for(die=0;die<ssd->parameter->die_chip;die++){
            for(plane=0;plane<ssd->parameter->plane_die;plane++){
                plane_flag = find_invalid_block(ssd, channel, chip, die, plane);
                counter=0;
                while(plane_flag==TRUE){
                    counter++;
                    //printf("There is data should be reorganize in %d, %d, %d, %d.\n",channel, chip, die, plane);
                    uninterrupt_dr(ssd, channel, chip, die, plane);
                    plane_flag = find_invalid_block(ssd, channel, chip, die, plane);
                }
                printf("SUCCESS. Data in %d, %d, %d, %d is reorganized.\n",channel, chip, die, plane);
            }
        }
    }
    return SUCCESS;
}


/************************************************************************************************************
*flag用来标记gc函数是在ssd整个都是idle的情况下被调用的（1），还是确定了channel，chip，die，plane被调用（0）
*进入gc函数，需要判断是否是不可中断的gc操作，如果是，需要将一整块目标block完全擦除后才算完成；
*如果是可中断的，在进行GC操作前，需要判断该channel，die是否有子请求在等待操作，如果没有则开始一步一步的操作，找到目标块后，一次执行一个copyback操作，跳出gc函数，待时间向前推进后，再做下一个copyback或者erase操作
*进入gc函数不一定需要进行gc操作，需要进行一定的判断，当处于硬阈值以下时，必须进行gc操作；当处于软阈值以下时，需要判断，看这个channel上是否有子请求在等待(有写子请求等待就不行，gc的目标die处于busy状态也不行)
*如果有就不执行gc，跳出，否则可以执行下一步操作
************************************************************************************************************/
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag)
{
    unsigned int i;
    int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
    unsigned int flag_priority=0;
    struct gc_operation *gc_node=NULL,*gc_p=NULL;

    if (flag==1)/*整个ssd都是IDEL的情况*/
    {
        for (i=0;i<ssd->parameter->channel_number;i++)
        {
            flag_priority=0;
            flag_direct_erase=1;
            flag_gc=1;
            flag_invoke_gc=1;
            gc_node=NULL;
            gc_p=NULL;
            if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))
            {
                channel=i;
                if (ssd->channel_head[channel].gc_command!=NULL)
                {
                    gc_for_channel(ssd, channel);
                }
            }
        }
        return SUCCESS;

    }
    else/*只需针对某个特定的channel，chip，die进行gc请求的操作(只需对目标die进行判定，看是不是idle）*/
    {
        if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
        {
            if ((ssd->channel_head[channel].subs_r_head!=NULL)||(ssd->channel_head[channel].subs_w_head!=NULL))    /*队列上有请求，先服务请求*/
            {
                return 0;
            }
        }

        return gc_for_channel(ssd,channel);
    }
}



/**********************************************************
*判断是否有子请求需要channel，若果没有返回1就可以发送gc操作
*如果有返回0，就不能执行gc操作，gc操作被中断
***********************************************************/
int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel)
{
    struct sub_request *sub;
    struct local *location;

    if ((ssd->channel_head[channel].subs_r_head==NULL)&&(ssd->channel_head[channel].subs_w_head==NULL))    /*这里查找读写子请求是否需要占用这个channel，不用的话才能执行GC操作*/
    {
        return 1;                                                                        /*表示当前时间这个channel没有子请求需要占用channel*/
    }
    else
    {
        if (ssd->channel_head[channel].subs_w_head!=NULL)
        {
            return 0;
        }
        else if (ssd->channel_head[channel].subs_r_head!=NULL)
        {
            sub=ssd->channel_head[channel].subs_r_head;
            while (sub!=NULL)
            {
                if (sub->current_state==SR_WAIT)                                         /*这个读请求是处于等待状态，如果他的目标die处于idle，则不能执行gc操作，返回0*/
                {
                    location=find_location(ssd,sub->ppn);
                    if ((ssd->channel_head[location->channel].chip_head[location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[location->channel].chip_head[location->chip].next_state==CHIP_IDLE)&&
                                                                                                                    (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)))
                    {
                        free(location);
                        location=NULL;
                        return 0;
                    }
                    free(location);
                    location=NULL;
                }
                else if (sub->next_state==SR_R_DATA_TRANSFER)
                {
                    location=find_location(ssd,sub->ppn);
                    if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)
                    {
                        free(location);
                        location=NULL;
                        return 0;
                    }
                    free(location);
                    location=NULL;
                }
                sub=sub->next_node;
            }
        }
        return 1;
    }
}