#include "ssd_group.h"
#include <liblightnvm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

static char nvm_dev_path[][NVM_DEV_PATH_LEN] = {"/dev/nvme0n1","/dev/nvme1n1","/dev/nvme2n1","/dev/nvme3n1","/dev/nvme4n1","/dev/nvme5n1","/dev/nvme6n1","/dev/nvme7n1","/dev/nvme8n1"};

int main(){
	int ch,lun,blk,pg;
	struct ssd_group *group = create_ssd_group(nvm_dev_path,9);
	if (!group){

		printf("create ssd_group error!!!\n");
		return 1;
	}
	ssd_group_pr(group);

	printf("please input location of page:(channel lun block page)\n");
	scanf("%d%d%d%d",&ch,&lun,&blk,&pg);

	struct ssd_group_addr page_addr;
	page_addr.ppa = 0;
	page_addr.g.ch = ch;
	page_addr.g.lun = lun;
	page_addr.g.blk = blk;
	page_addr.g.pg = pg;
	page_addr.g.pth = 0;

	struct nvm_ret *ret;
	struct ssd_group_geo *geo = ssd_group_geo_get(group);
	int naddrs = geo->nplanes * geo->nsectors;
	int buf_nbytes = naddrs * geo->sector_nbytes;
	int page_num = 64;
        int buf_nbytes_g = buf_nbytes * page_num;
	printf ("buf_nbytes_g :%d\n",buf_nbytes_g); 

	struct nvm_geo *geo_ = ssd_group_origin_geo_get(group);

	char *buf_w_g = nvm_buf_alloc(geo_, buf_nbytes_g);
	char *buf_r_g = nvm_buf_alloc(geo_, buf_nbytes_g);
	char *buf_w_ = nvm_buf_alloc(geo_, buf_nbytes);
	char *buf_r_ = nvm_buf_alloc(geo_, buf_nbytes);

	if (!buf_w_g) { 
		printf("alloc buf_w_g error!\n");
		return 0;
	}
	if (!buf_r_g) { 
		printf("alloc buf_r_g error!\n");
		return 0;
	}

	for (int i = 0; i < buf_nbytes_g; ++i) {
		buf_w_g[i] = 65 + (i / buf_nbytes) % 26; 
	}
	
	struct timeval tv;
	long long be;
	long long e_time = 0;

	long long w_time = 0;
	long long r_time = 0;
	int res = -1;
	void *a;
	pthread_t t;
	struct para_g *para = malloc(sizeof(struct para_g));
	for (int j = 0; j < ((page_num-1)/geo->npages)+1; j++) {
		printf("j : + %d\n",j);
		if (blk + j >= geo->nblocks)
			return 1;
		page_addr.g.blk = blk + j;
		gettimeofday(&tv,NULL);
		be = tv.tv_sec*1000000 + tv.tv_usec;
		para->group_ = group;
		para->page_addr_ = page_addr;
		para->geo_ = geo;
		para->buf_g_ = NULL;
		para->ret_ = ret;
		ssd_erase_struct(para);
		gettimeofday(&tv,NULL);
		e_time += tv.tv_sec*1000000 + tv.tv_usec - be;
	
		for (int i = 0; i < geo->npages ;i++){
			if (i >= page_num)
				break;
			page_addr.g.pg = i;
			
			gettimeofday(&tv,NULL);
			for (int k = 0; k < buf_nbytes; k++)
			{
				long long temp2 = (j * geo->npages  + i) * buf_nbytes;
				buf_w_[k] = buf_w_g[temp2 + k];
			}
			
			be = tv.tv_sec*1000000 + tv.tv_usec;
			para->group_ = group;
			para->page_addr_ = page_addr;
			para->geo_ = geo;
			para->buf_g_ = buf_w_;
			para->ret_ = ret;
			ssd_write_struct(para);
			gettimeofday(&tv,NULL);
			w_time += tv.tv_sec*1000000 + tv.tv_usec - be;
	
			gettimeofday(&tv,NULL);
			be = tv.tv_sec*1000000 + tv.tv_usec;
			para->group_ = group;
			para->page_addr_ = page_addr;
			para->geo_ = geo;
			para->buf_g_ = buf_r_;
			para->ret_ = ret;
			ssd_read_struct(para);
			gettimeofday(&tv,NULL);
			r_time += tv.tv_sec*1000000 + tv.tv_usec - be;
		}
	}
	printf("e_time: %lld\n", e_time);
	printf("w_time: %lld\n", w_time);
	printf("r_time: %lld\n", r_time);
	
	free(buf_w_g);
	free(buf_w_);
	free(buf_r_g);
	free(buf_r_);
	free(para);

	free_ssd_group(group);
	return 0;
}
