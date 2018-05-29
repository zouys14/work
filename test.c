#include "ssd_group.h"
#include <liblightnvm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static char nvm_dev_path[][NVM_DEV_PATH_LEN] = {"/dev/nvme0n1","/dev/nvme1n1","/dev/nvme2n1","/dev/nvme3n1","/dev/nvme4n1","/dev/nvme5n1","/dev/nvme6n1","/dev/nvme7n1","/dev/nvme8n1"};

int main(){
	struct ssd_group_thread *ssd_thread = create_ssd_group_pthread(nvm_dev_path,9,3);
	if (!ssd_thread){

		printf("create ssd_group error!!!\n");
		return 1;
	}
	ssd_group_pr_pthread(ssd_thread);

	struct nvm_geo *geo = nvm_dev_get_geo(ssd_thread->ssd_group_list[0]->ssd_list[0]);
	printf("nchannels:%d,nluns:%d,nplanes:%d,nblocks:%d,npages:%d,nsectors:%d\n",geo->nchannels,geo->nluns,geo->nplanes,geo->nblocks,geo->npages,geo->nsectors);
	printf("please input location of block:(ch lun blk)\n");
	int ch,lun,blk;
	scanf("%d%d%d",&ch,&lun,&blk);

	static struct nvm_addr blk_addr;
	blk_addr.ppa = 0;
	blk_addr.g.ch = ch;
	blk_addr.g.lun = lun;
	blk_addr.g.blk = blk;

	struct nvm_ret *ret;
	int naddrs = geo->nplanes * geo->nsectors;
	int buf_nbytes = naddrs * geo->sector_nbytes;
        int buf_nbytes_g =buf_nbytes * ssd_thread->thread_number;
	printf ("buf_nbytes :%d + buf_nbytes_g :%d\n", buf_nbytes, buf_nbytes_g); 

	char *buf_w_g = nvm_buf_alloc(geo, buf_nbytes_g);
	char *buf_r_g = nvm_buf_alloc(geo, buf_nbytes_g);

	if (!buf_w_g) { 
		printf("alloc buf_w_g error!\n");
		return 0;
	}
	if (!buf_r_g) { 
		printf("alloc buf_r_g error!\n");
		return 0;
	}

	nvm_buf_fill(buf_w_g, buf_nbytes_g);
	printf ("buf_w_g: \n");
	for( int i = 0; i < 26 * 2; ++i) 
			printf("%c ",buf_w_g[i]);
	printf("\n");

	int res = ssd_group_erase_pthread(ssd_thread, blk_addr, geo, ret);
	if (res > 0)
		printf ("erase ssd_group_pthread sucessful!\n");

	
	res = ssd_group_write_pthread(ssd_thread, blk_addr, geo, buf_w_g, ret);
	if (res > 0)
		printf("write ssd_group_pthread sucessful!\n");
	else
		printf("write ssd_group_pthread failed!\n");
	res = ssd_group_read_pthread(ssd_thread, blk_addr, geo, buf_r_g, ret);
	if (res > 0)
		printf("read ssd_group_pthread successful!\n");
	else
		printf("read ssd_group_pthread failed!\n");
	free(buf_w_g);
	free(buf_r_g);

	free_ssd_group_pthread(ssd_thread);
	return 0;
}
