#include "ssd_group.h"
#include <liblightnvm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char nvm_dev_path[][NVM_DEV_PATH_LEN] = {"/dev/nvme0n1","/dev/nvme1n1","/dev/nvme2n1"};

int main(){
	struct ssd_group *group = create_ssd_group(nvm_dev_path,3);
	if (!group){
		printf("create ssd_group error!!!\n");
		return 1;
	}
	ssd_group_pr(group);

	struct nvm_geo *geo = nvm_dev_get_geo(group->ssd_list[0]);
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

	int res = ssd_group_erase(group, blk_addr, geo, ret);
	if (res > 0)
		printf ("erase ssd_group sucessful!\n");

	res = ssd_group_write(group, blk_addr, geo, ret);
	if (res > 0)
		printf ("write ssd_group sucessful!\n");
	else
		printf ("write ssd_group failed!\n");
	
	res = ssd_group_read(group, blk_addr, geo, ret);
	if (res > 0)
		printf ("read ssd_group sucessful!\n");
	else
		printf ("read ssd_group failed!\n");

	free_ssd_group(group);
	return 0;
}
