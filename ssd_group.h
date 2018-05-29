#ifndef _SSD_GROUP_H
#define _SSD_GROUP_H

#include <liblightnvm.h>
#include <pthread.h>

#define SSD_MAX_NUMBER 16
#define SSD_PROC 4

struct ssd_group{
	struct nvm_dev *ssd_list[SSD_MAX_NUMBER];
	int ssd_number;
};

struct ssd_group_thread{
	struct ssd_group *ssd_group_list[SSD_PROC];
	int thread_number;
};

struct para_g{
	struct ssd_group *group_;
	struct nvm_addr blk_addr_;
	struct nvm_geo *geo_;
	char *buf_g_;
	struct nvm_ret *ret_;
};

struct ssd_group *create_ssd_group(char path[][NVM_DEV_PATH_LEN],int num);

struct ssd_group *create_ssd_group_(char path[][NVM_DEV_PATH_LEN],int bgn,int end);

struct ssd_group_thread *create_ssd_group_pthread(char path[][NVM_DEV_PATH_LEN], int num1, int num2);

void ssd_group_pr(const struct ssd_group *group);

void ssd_group_pr_pthread(const struct ssd_group_thread *group_thread);

void free_ssd_group(struct ssd_group *group);

void free_ssd_group_pthread(struct ssd_group_thread *group_thread);

int ssd_group_erase(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, struct nvm_ret *ret);

int ssd_group_write(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_w, struct nvm_ret *ret);

int ssd_group_read(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_r, struct nvm_ret *ret);

int ssd_group_erase_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, struct nvm_ret *ret);

int ssd_group_write_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_w_g, struct nvm_ret *ret);

int ssd_group_read_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_r_g, struct nvm_ret *ret);

int ssd_group_write_struct(void *para_struct);

int ssd_group_read_struct(void *para_struct);

#endif
