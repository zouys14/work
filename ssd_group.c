#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <pthread.h>
#include "ssd_group.h"

struct ssd_group *create_ssd_group(char path[][NVM_DEV_PATH_LEN],int num)
{
	struct ssd_group *group;
	group = (struct ssd_group *) malloc(sizeof(struct ssd_group));
	for (int i = 0; i < num; i++){
		group->ssd_list[i] = nvm_dev_open(path[i]);
		if (!group->ssd_list[i]) {
			printf("nvm_dev_open %s error!\n",path[i]);
			return NULL;
		}
		printf("nvm_dev_open %s sucessful!\n",path[i]);
	}
	struct nvm_geo *geo = nvm_dev_get_geo(group->ssd_list[0]);
	for (int i = 0; i < num; ++i)
		strcpy(group->group_dev_path[i],path[i]);

	group->group_geo =(struct ssd_group_geo *) malloc(sizeof(struct ssd_group_geo)); 
	
	group->group_geo->nthreads = num / DEFAULT_DEV_NUMBER;
	group->group_geo->nchannels = geo->nchannels;	
	group->group_geo->nluns = geo->nluns;		
	group->group_geo->nplanes = geo->nplanes;		
	group->group_geo->nblocks = geo->nblocks;		
	group->group_geo->npages = geo->npages;		
	group->group_geo->nsectors = geo->nsectors;	

	group->group_geo->page_nbytes = geo->page_nbytes;	
	group->group_geo->sector_nbytes = geo->sector_nbytes;	
	group->group_geo->meta_nbytes = geo->meta_nbytes;	

	group->group_geo->tbytes = (long long)geo->tbytes * group-> group_geo->nthreads;

	group->p_mode = nvm_dev_get_pmode(group->ssd_list[0]);
	group->meta_mode = nvm_dev_get_meta_mode(group->ssd_list[0]);
	return group;
}

const struct ssd_group_geo *ssd_group_geo_get(const struct ssd_group *group)
{
	return group->group_geo;
}

int ssd_group_pmode_get(const struct ssd_group *group)
{
	return group->p_mode;
}

int ssd_group_meta_mode_get(const struct ssd_group *group)
{
	return group->meta_mode;
}

const struct nvm_geo * ssd_group_origin_geo_get(const struct ssd_group *group)
{
	return nvm_dev_get_geo(group->ssd_list[0]);
}

void ssd_group_pr(const struct ssd_group *group)
{
	printf("ssd group geo : \n");
	printf("nchannels : %d\n",group->group_geo->nchannels);
	printf("nluns : %d\n",group->group_geo->nluns);
	printf("nplanes : %d\n",group->group_geo->nplanes);
	printf("nblocks : %d\n",group->group_geo->nblocks);
	printf("npages : %d\n",group->group_geo->npages);
	printf("nsectors : %d\n",group->group_geo->nsectors);
	printf("page_nbytes : %d\n",group->group_geo->page_nbytes);
	printf("sector_nbytes : %d\n",group->group_geo->sector_nbytes);
	printf("meta_nbytes : %d\n",group->group_geo->meta_nbytes);
	printf("tbytes : %lld\n",group->group_geo->tbytes);		
	printf("ssd group mode : \n");
	printf("pmode : %d\n",group->p_mode);
	printf("metamode : %d\n",group->meta_mode);
}

void free_ssd_group(struct ssd_group *group)
{
	int num = group->group_geo->nthreads * DEFAULT_DEV_NUMBER;
	for (int i = 0; i < num; i++){
		nvm_dev_close(group->ssd_list[i]);
	}
	free(group->group_geo);
	free(group);	
}

int ssd_group_erase(struct ssd_group *group, struct ssd_group_addr page_addr, struct nvm_ret *ret)
{
	struct ssd_group_geo *geo = ssd_group_geo_get(group); 
	int naddrs = geo->nthreads;
	
	struct ssd_group_addr addrs[naddrs];
	struct para_g para[naddrs];

	for (int i = 0; i < naddrs; ++i){
		addrs[i].ppa = page_addr.ppa;
		addrs[i].g.pth = i;
		
		para[i].group_ = group;
		para[i].page_addr_ = addrs[i];
		para[i].geo_ = geo;
		para[i].buf_g_ = NULL;
		para[i].ret_ = ret;
	}

	pthread_t t1,t2,t3;
	int ret1,ret2,ret3;
	
	ret1 = pthread_create(&t1,NULL,ssd_group_erase_struct,&(para[0]));
	if (ret1 != 0)
		printf("erase pthread 0 error!\n");
	ret2 = pthread_create(&t2,NULL,ssd_group_erase_struct,&(para[1]));
	if (ret2 != 0)
		printf("erase pthread 1 error!\n");
	ret3 = pthread_create(&t3,NULL,ssd_group_erase_struct,&(para[2]));
	if (ret3 != 0)
		printf("erase pthread 2 error!\n");
	void *a1, *a2, *a3;
	pthread_join(t1,&a1);
	pthread_join(t2,&a2);
	pthread_join(t3,&a3);

	if ((int *)a1 > 0 && (int *)a2 > 0 && (int *)a3 > 0)
		return 1;
	return 0;
}

int ssd_group_write(struct ssd_group *group, struct ssd_group_addr page_addr, char *buf_w, struct nvm_ret *ret)
{
	printf("begin ssd group write!!!\n");
	struct ssd_group_geo *geo = ssd_group_geo_get(group); 
	int naddrs = geo->nthreads;
	
	struct ssd_group_addr addrs[naddrs];
	struct para_g para[naddrs];
	
	int buf_nbytes = geo->nplanes * geo->nsectors * geo->sector_nbytes;
	int buf_nbytes_g = buf_nbytes * naddrs;
	char **buf_w_ = malloc(sizeof(char *) * naddrs);
	for (int i = 0; i < naddrs; ++i)
		buf_w_[i] = nvm_buf_alloc(ssd_group_origin_geo_get(group), buf_nbytes);
	
	for (int i = 0; i < buf_nbytes_g; ++i) {
		buf_w_[i/buf_nbytes][i%buf_nbytes] = buf_w[i];
	}

	for (int i = 0; i < naddrs; ++i){
		addrs[i].ppa = page_addr.ppa;
		addrs[i].g.pth = i;
		
		para[i].group_ = group;
		para[i].page_addr_ = addrs[i];
		para[i].geo_ = geo;
		para[i].buf_g_ = buf_w_[i];
		para[i].ret_ = ret;
	}

	pthread_t t1,t2,t3;
	int ret1,ret2,ret3;
	ret1 = pthread_create(&t1,NULL,ssd_group_write_struct,&(para[0]));
	if (ret1 != 0)
		printf("write pthread 0 error!\n");
	ret2 = pthread_create(&t2,NULL,ssd_group_write_struct,&(para[1]));
	if (ret2 != 0)
		printf("write pthread 1 error!\n");
	ret3 = pthread_create(&t3,NULL,ssd_group_write_struct,&(para[2]));
	if (ret3 != 0)
		printf("write pthread 2 error!\n");
	void *a1, *a2, *a3;
	pthread_join(t1,&a1);
	pthread_join(t2,&a2);
	pthread_join(t3,&a3);
	
	for (int i = 0; i < naddrs; ++i)
		free(buf_w_[i]);
	free(buf_w_);

	if ((int *)a1 > 0 && (int *)a2 > 0 && (int *)a3 > 0)
		return 1;
	return 0;
}

int ssd_group_read(struct ssd_group *group, struct ssd_group_addr page_addr, char *buf_r, struct nvm_ret *ret)
{
	struct ssd_group_geo *geo = ssd_group_geo_get(group); 
	int naddrs = geo->nthreads;
	
	struct ssd_group_addr addrs[naddrs];
	struct para_g para[naddrs];
	
	int buf_nbytes = geo->nplanes * geo->nsectors * geo->sector_nbytes;
	int buf_nbytes_g = buf_nbytes * naddrs;
	char **buf_r_ = malloc(sizeof(char *) * naddrs);
	for (int i = 0; i < naddrs; ++i){
		buf_r_[i] = nvm_buf_alloc(ssd_group_origin_geo_get(group), buf_nbytes);
		memset(buf_r_[i], 0 , buf_nbytes);
	}

	for (int i = 0; i < naddrs; ++i){
		addrs[i].ppa = page_addr.ppa;
		addrs[i].g.pth = i;
		
		para[i].group_ = group;
		para[i].page_addr_ = addrs[i];
		para[i].geo_ = geo;
		para[i].buf_g_ = buf_r_[i];
		para[i].ret_ = ret;
	}

	pthread_t t1,t2,t3;
	int ret1,ret2,ret3;
	
	ret1 = pthread_create(&t1,NULL,ssd_group_read_struct,&(para[0]));
	if (ret1 != 0)
		printf("read pthread 0 error!\n");
	ret2 = pthread_create(&t2,NULL,ssd_group_read_struct,&(para[1]));
	if (ret2 != 0)
		printf("read pthread 1 error!\n");
	ret3 = pthread_create(&t3,NULL,ssd_group_read_struct,&(para[2]));
	if (ret3 != 0)
		printf("read pthread 2 error!\n");
	void *a1, *a2, *a3;
	pthread_join(t1,&a1);
	pthread_join(t2,&a2);
	pthread_join(t3,&a3);
	
	for (int i = 0; i < buf_nbytes_g; ++i) {
		buf_r[i] = buf_r_[i/buf_nbytes][i%buf_nbytes];
	}	
	for (int i = 0; i < naddrs; ++i)
		free(buf_r_[i]);
	free(buf_r_);

	if ((int *)a1 > 0 && (int *)a2 > 0 && (int *)a3 > 0)
		return 1;
	return 0;
}

int ssd_group_erase_struct(void *para_struct)
{
	struct para_g *para = (struct para_g *) para_struct;
	struct ssd_group *group = (*para).group_;
	struct ssd_group_addr page_addr = (*para).page_addr_;
	struct ssd_group_geo *geo = (*para).geo_;
	char *buf_g = (*para).buf_g_;
	struct nvm_ret *ret = (*para).ret_;
	
	struct nvm_addr addr;
	addr.ppa = page_addr.ppa;
	addr.g.blk = (addr.g.blk) / 4;
	int thread = page_addr.g.pth;
	
	int pmode = ssd_group_pmode_get(group);
	int naddrs = geo->nplanes;
	struct nvm_addr addrs[naddrs];
	
	if (pmode) {
		addrs[0].ppa = addr.ppa;
	} else {
		for (int pl = 0; pl < geo->nplanes; ++pl) {
			addrs[pl].ppa = addr.ppa;
			addrs[pl].g.pl = pl;
		}
	}

	int res = -1;
	for (int i = 0; i < DEFAULT_DEV_NUMBER; i++){
		res = nvm_addr_erase(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER], addrs, pmode ? 1 : geo->nplanes, pmode, ret);
		if (res < 0){
			printf("erase %s error!\n", nvm_dev_get_name(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER]));
			return 0;
		}
	}
	return 1;
}

int ssd_group_write_struct(void *para_struct)
{
	struct para_g *para = (struct para_g *) para_struct;
	struct ssd_group *group = (*para).group_;
	struct ssd_group_addr page_addr = (*para).page_addr_;
	struct ssd_group_geo *geo = (*para).geo_;
	char *buf_w = (*para).buf_g_;
	struct nvm_ret *ret = (*para).ret_;

	struct nvm_addr addr;
	addr.ppa = page_addr.ppa;
	addr.g.blk = (addr.g.blk) / 4;
	int thread = page_addr.g.pth;
	
	int pmode = ssd_group_pmode_get(group);
	int naddrs = geo->nplanes * geo->nsectors;
	int use_meta = ssd_group_meta_mode_get(group);
	struct nvm_addr addrs[naddrs];

	char *meta_w = NULL;
	int meta_nbytes = naddrs * geo->meta_nbytes;
	meta_w = nvm_buf_alloc(ssd_group_origin_geo_get(group), meta_nbytes);
	if (!meta_w) {
		printf("alloc meta_w error!\n");
		return 0;
	}
	for (int i = 0; i < meta_nbytes; ++i) {
		meta_w[i] = 65;
	}
	
	for (int i = 0; i < naddrs; ++i) {
		char meta_descr[meta_nbytes];
		int sec = i % geo->nsectors;
		int pl = (i / geo->nsectors) % geo->nplanes;

		sprintf(meta_descr, "[P(%02d),S(%02d)]", pl, sec);
		if (strlen(meta_descr) > geo->meta_nbytes) {
			printf("meta_descr error!\n");
			return 0;
		}

		memcpy(meta_w + i * geo->meta_nbytes, meta_descr,
		       strlen(meta_descr));
	}
	for (int i = 0; i < naddrs; ++i) {
		addrs[i].ppa = addr.ppa;
		addrs[i].g.sec = i % geo->nsectors;
		addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
	}

	int res = -1;
	for (int i = 0; i < DEFAULT_DEV_NUMBER; i++){
		res = nvm_addr_write(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER], addrs, naddrs, buf_w, use_meta ? meta_w : NULL, pmode, ret);
		if (res < 0) {
			printf("write data error in dev %s!\n",nvm_dev_get_name(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER]));
			return 0;
		}
		else
			printf("write data sucessful in dev %s!\n",nvm_dev_get_name(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER]));
	}
	return 1;
}

int ssd_group_read_struct(void *para_struct)
{
	struct para_g *para = (struct para_g *) para_struct;
	struct ssd_group *group = (*para).group_;
	struct ssd_group_addr page_addr = (*para).page_addr_;
	struct ssd_group_geo *geo = (*para).geo_;
	char *buf_r = (*para).buf_g_;
	struct nvm_ret *ret = (*para).ret_;
	
	struct nvm_addr addr;
	addr.ppa = page_addr.ppa;
	addr.g.blk = (addr.g.blk) / 4;
	int thread = page_addr.g.pth;
	
	int pmode = ssd_group_pmode_get(group);
	int naddrs = geo->nplanes * geo->nsectors;
	int use_meta = ssd_group_meta_mode_get(group);
	struct nvm_addr addrs[naddrs];

	char *meta_r = NULL;	
	int buf_nbytes = naddrs * geo->sector_nbytes;
	int meta_nbytes = naddrs * geo->meta_nbytes;

	meta_r = nvm_buf_alloc(ssd_group_origin_geo_get(group), meta_nbytes);
	if (!meta_r) {
		printf("alloc meta_r error!\n");
		return 0;
	}

	for (int i = 0; i < naddrs; ++i) {
		addrs[i].ppa = addr.ppa;
		addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
		addrs[i].g.sec = i % geo->nsectors;
	}

	memset(buf_r, 0, buf_nbytes);
	if (use_meta)
		memset(meta_r, 0 , meta_nbytes);
	int res = -1;
	for (int i = 0; i < DEFAULT_DEV_NUMBER; ++i){
		res = nvm_addr_read(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER], addrs, naddrs, buf_r, use_meta ? meta_r : NULL, pmode, ret);
		if (res < 0 && i < DEFAULT_DEV_NUMBER - 1) 
			printf("read error in dev %s!\n",  nvm_dev_get_name(group->ssd_list[i + thread * DEFAULT_DEV_NUMBER]));
		else if (res < 0 && i == DEFAULT_DEV_NUMBER - 1){
			printf("read error in all dev!\n");
			return 0;
		}
		else if (res == 0){
			break;
		}
	}
	return 1;
}

