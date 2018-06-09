#ifndef _SSD_GROUP_H
#define _SSD_GROUP_H

#include <liblightnvm.h>
#include <pthread.h>

#define SSD_MAX_NUMBER 16
#define SSD_PROC 4
#define DEFAULT_DEV_NUMBER 3

struct ssd_group_addr {
	union {
		struct {
			uint64_t pth	: 2;
			uint64_t blk	: 14;
			uint64_t pg	: 16;
			uint64_t sec	: 8;
			uint64_t pl	: 8;
			uint64_t lun	: 8;
			uint64_t ch	: 7;	
			uint64_t rsvd	: 1;
		} g;

		uint64_t ppa;
	};
};

struct ssd_group_geo {
	size_t nthreads;
	size_t nchannels;	
	size_t nluns;		
	size_t nplanes;		
	size_t nblocks;		
	size_t npages;		
	size_t nsectors;	

	size_t page_nbytes;	
	size_t sector_nbytes;	
	size_t meta_nbytes;	

	long long tbytes;		
};

struct ssd_group{
	char group_dev_path[SSD_MAX_NUMBER][NVM_DEV_PATH_LEN];
	struct nvm_dev *ssd_list[SSD_MAX_NUMBER];
	struct ssd_group_geo *group_geo;
	int p_mode;
	int meta_mode;
};

struct para_g{
	struct ssd_group *group_;
	struct ssd_group_addr page_addr_;
	struct ssd_group_geo *geo_;
	char *buf_g_;
	struct nvm_ret *ret_;
};

struct ssd_group *create_ssd_group(char path[][NVM_DEV_PATH_LEN],int num);

const struct ssd_group_geo *ssd_group_geo_get(const struct ssd_group *group);

const struct nvm_geo *ssd_group_origin_geo_get(const struct ssd_group *group);

int ssd_group_pmode_get(const struct ssd_group *group);

int ssd_group_meta_mode_get(const struct ssd_group *group);

void ssd_group_pr(const struct ssd_group *group);

void free_ssd_group(struct ssd_group *group);

int ssd_group_erase(struct ssd_group *group, struct ssd_group_addr page_addr, struct nvm_ret *ret);

int ssd_group_write(struct ssd_group *group, struct ssd_group_addr page_addr, char *buf_w, struct nvm_ret *ret);

int ssd_group_read(struct ssd_group *group, struct ssd_group_addr page_addr, char *buf_r, struct nvm_ret *ret);

int ssd_group_erase_struct(void *para_strust);

int ssd_group_write_struct(void *para_struct);

int ssd_group_read_struct(void *para_struct);

int ssd_erase_struct(struct para_g *para);

int ssd_write_struct(struct para_g *para);

int ssd_read_struct(struct para_g *para);

int ssd_group_error_handle(struct ssd_group *group, struct nvm_dev *dev, struct ssd_group_addr blk_addr, struct nvm_ret *ret);

#endif
