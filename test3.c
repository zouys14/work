#include "ssd_group.h"
#include <liblightnvm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


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

	struct nvm_ret *ret;
	struct ssd_group_geo *geo = ssd_group_geo_get(group);
	int naddrs = geo->nplanes * geo->nsectors;
	int buf_nbytes = naddrs * geo->sector_nbytes;
        int buf_nbytes_g =buf_nbytes;
	printf ("buf_nbytes_g :%d\n",buf_nbytes_g); 

	struct nvm_geo *geo_ = ssd_group_origin_geo_get(group);

	char *buf_w_g = nvm_buf_alloc(geo_, buf_nbytes_g);

	if (!buf_w_g) { 
		printf("alloc buf_w_g error!\n");
		return 0;
	}

	for (int i = 0; i < buf_nbytes_g; ++i) {
		buf_w_g[i] = 65 + i / buf_nbytes; 
	}
	printf ("buf_w_g: \n");
	for( int i = 0; i < 1; ++i) 
		printf("char %d:%c + char %d:%c \n", i * buf_nbytes , buf_w_g[i * buf_nbytes], i * buf_nbytes + 1, buf_w_g[i * buf_nbytes + 1]);
	printf("\n");

	int res = ssd_group_erase(group, page_addr, ret);
	if (res > 0)
		printf ("erase ssd_group sucessful!\n");

	
	res = ssd_group_write(group, page_addr, buf_w_g, ret);
	if (res > 0)
		printf("write ssd_group sucessful!\n");
	else
		printf("write ssd_group failed!\n");
	
	
	free(buf_w_g);
	free_ssd_group(group);
	return 0;
}
