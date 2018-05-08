#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include "ssd_group.h"

struct ssd_group *create_ssd_group(char path[][NVM_DEV_PATH_LEN],int num)
{
	struct ssd_group *group;
	group = malloc(sizeof(* group));
	group->ssd_number = num;
	for (int i = 0; i < num; i++){
		group->ssd_list[i] = nvm_dev_open(path[i]);
		if (!group->ssd_list[i]) {
			printf("nvm_dev_open %s error!\n",path[i]);
			return NULL;
		}
		printf("nvm_dev_open %s sucessful!\n",path[i]);
	}
	return group;
}

void ssd_group_pr(const struct ssd_group *group)
{
	int num = group->ssd_number;
	for (int i = 0; i < num; i++){
		printf("ssd %d:%s\n",i+1,nvm_dev_get_name(group->ssd_list[i]));
	}
}

void free_ssd_group(struct ssd_group *group)
{
	int num = group->ssd_number;
	for (int i = 0; i < num; i++){
		nvm_dev_close(group->ssd_list[i]);
	}
	free(group);	
}

int ssd_group_erase(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, struct nvm_ret *ret)
{
	int naddrs = geo->nplanes * geo->nsectors;
	int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
	struct nvm_addr addrs[naddrs];

	if (pmode) {
		addrs[0].ppa = blk_addr.ppa;
	} else {
		for (int pl = 0; pl < geo->nplanes; ++pl) {
			addrs[pl].ppa = blk_addr.ppa;

			addrs[pl].g.pl = pl;
		}
	}
	int num = group->ssd_number;
	for (int i = 0; i < num; i++){
		int res = nvm_addr_erase(group->ssd_list[i], addrs, pmode ? 1 : geo->nplanes, pmode, ret);
		if (res < 0){
			printf("erase %s error!\n", nvm_dev_get_name(group->ssd_list[i]));
			return 0;
		}
	}
	return 1;
}
	
