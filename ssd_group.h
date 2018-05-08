#ifndef _SSD_GROUP_H
#define _SSD_GROUP_H

#include <liblightnvm.h>

#define SSD_MAX_NUMBER 16

struct ssd_group{
	struct nvm_dev *ssd_list[SSD_MAX_NUMBER];
	int ssd_number;
};

struct ssd_group *create_ssd_group(char path[][NVM_DEV_PATH_LEN],int num);

void ssd_group_pr(const struct ssd_group *group);

void free_ssd_group(struct ssd_group *group);

int ssd_group_erase(struct ssd_group *group, struct nvm_addr addr, struct nvm_geo *geo, struct nvm_ret *ret);

#endif
