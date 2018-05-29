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

struct ssd_group *create_ssd_group_(char path[][NVM_DEV_PATH_LEN],int bgn,int end)
{
	struct ssd_group *group;
	group = malloc(sizeof(* group));
	group->ssd_number = end - bgn;
	printf ("begin : %d + end : %d \n",bgn,end);
	for (int i = bgn; i < end; i++){
		group->ssd_list[i % (group->ssd_number)] = nvm_dev_open(path[i]);
		if (!group->ssd_list[i % (group->ssd_number)]) {
			printf("nvm_dev_open %s error!\n",path[i]);
			return NULL;
		}
		printf("nvm_dev_open %s sucessful!\n",path[i]);
	}
	return group;
}

struct ssd_group_thread *create_ssd_group_pthread(char path[][NVM_DEV_PATH_LEN], int num1, int num2)
{
	struct ssd_group_thread *ssd_thread;
	ssd_thread = malloc(sizeof(* ssd_thread));

	ssd_thread->thread_number = num2;
	int each_num = num1/num2;
	for (int i = 0; i < num2; i++){
		ssd_thread->ssd_group_list[i] = create_ssd_group_(path, i*each_num , (i+1)*each_num);
		if (ssd_thread->ssd_group_list[i] == NULL)
			return NULL;
	}
	printf("in func create!!!\n");
	for (int i = 0; i < num2; i++)
		ssd_group_pr(ssd_thread->ssd_group_list[i]);
	return ssd_thread;
}

void ssd_group_pr(const struct ssd_group *group)
{
	int num = group->ssd_number;
	for (int i = 0; i < num; i++){
		printf("ssd %d:%s\n",i+1,nvm_dev_get_name(group->ssd_list[i]));
	}
}

void ssd_group_pr_pthread(const struct ssd_group_thread *group_thread)
{
	int num = group_thread->thread_number;
	for (int i = 0; i < num; i++){
		printf("thread %d : \n",i);
		ssd_group_pr(group_thread->ssd_group_list[i]);
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

void free_ssd_group_pthread(struct ssd_group_thread *group_thread)
{
	int num = group_thread->thread_number;
	for (int i = 0; i < num; i++)
		free_ssd_group(group_thread->ssd_group_list[i]);
	free(group_thread);
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

int ssd_group_write(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_w, struct nvm_ret *ret)
{
	int naddrs = geo->nplanes * geo->nsectors;
	int use_meta = 0;
	int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
	struct nvm_addr addrs[naddrs];
	char *meta_w = NULL;

	int meta_nbytes = naddrs * geo->meta_nbytes;

	meta_w = nvm_buf_alloc(geo, meta_nbytes);
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
	
	for (int pg = 0; pg < geo->npages; ++pg) {
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.sec = i % geo->nsectors;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
		}

		int num = group->ssd_number;
		int res = -1;
		for (int j = 0; j < num; j++){
			res = nvm_addr_write(group->ssd_list[j], addrs, naddrs, buf_w, use_meta ? meta_w : NULL, pmode, ret);
			if (res < 0) {
				printf("write data error in dev %s!\n",nvm_dev_get_name(group->ssd_list[j]));
				return 0;
			}
		}
	}
	return 1;
}

int ssd_group_read(struct ssd_group *group, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_r, struct nvm_ret *ret)
{
	int naddrs = geo->nplanes * geo->nsectors;
	int use_meta = 0;
	int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
	struct nvm_addr addrs[naddrs];
	char *meta_r = NULL;
	
	int buf_nbytes = naddrs * geo->sector_nbytes;
	int meta_nbytes = naddrs * geo->meta_nbytes;

	meta_r = nvm_buf_alloc(geo, meta_nbytes);
	if (!meta_r) {
		printf("alloc meta_r error!\n");
		return 0;
	}

	for (int pg = 0; pg < geo->npages; ++pg) {
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
			addrs[i].g.sec = i % geo->nsectors;
		}

		memset(buf_r, 0, buf_nbytes);
		if (use_meta)
			memset(meta_r, 0 , meta_nbytes);
		int num = group->ssd_number;
		int res = -1;
		for (int j = 0; j < num; j++){
			res = nvm_addr_read(group->ssd_list[j], addrs, naddrs, buf_r, use_meta ? meta_r : NULL, pmode, ret);
			if (res < 0 && j < num-1) 
				printf("read page %d error in dev %s!\n", pg, nvm_dev_get_name(group->ssd_list[j]));
			else if (res < 0 && j == num-1){
				printf("read page %d error in all dev!\n", pg);
				return 0;
			}
			else if (res == 0){
				break;
			}
		}
	}
	return 1;
}

int ssd_group_write_struct(void *para_struct)
{
	struct para_g *para = (struct para_g *) para_struct;
	struct ssd_group *group = (*para).group_;
	struct nvm_addr blk_addr = (*para).blk_addr_;
	struct nvm_geo *geo = (*para).geo_;
	char *buf_g = (*para).buf_g_;
	struct nvm_ret *ret = (*para).ret_;
	return (ssd_group_write(group, blk_addr, geo, buf_g, ret));
}

int ssd_group_read_struct(void *para_struct)
{
	struct para_g *para = (struct para_g *) para_struct;
	struct ssd_group *group = (*para).group_;
	struct nvm_addr blk_addr = (*para).blk_addr_;
	struct nvm_geo *geo = (*para).geo_;
	char *buf_g = (*para).buf_g_;
	struct nvm_ret *ret = (*para).ret_;
	return (ssd_group_read(group, blk_addr, geo, buf_g, ret));
}

int ssd_group_erase_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, struct nvm_ret *ret)
{
	int num = group_thread->thread_number;
	for (int i = 0; i < num; i++){
		if (ssd_group_erase(group_thread->ssd_group_list[i], blk_addr, geo, ret) == 0)
			return 0;
	}
	return 1;
}

int ssd_group_write_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_w_g, struct nvm_ret *ret)
{
	pthread_t t1,t2,t3;
	int ret1,ret2,ret3;
	int naddrs = geo->nplanes * geo->nsectors;
	int buf_nbytes = naddrs * geo->sector_nbytes;
	char *buf_w_1 = nvm_buf_alloc(geo, buf_nbytes);
	char *buf_w_2 = nvm_buf_alloc(geo, buf_nbytes);
	char *buf_w_3 = nvm_buf_alloc(geo, buf_nbytes);
	for (int i = 0; i < buf_nbytes; ++i) {
		buf_w_1[i] = buf_w_g[i*3];
		buf_w_2[i] = buf_w_g[i*3+1];
		buf_w_3[i] = buf_w_g[i*3+2];
	}
	struct para_g para1,para2,para3;
	para1.group_ = group_thread->ssd_group_list[0];
	para1.blk_addr_ = blk_addr;
	para1.geo_ = geo;
	para1.buf_g_ = buf_w_1;
	para1.ret_ = ret;
	
	para2.group_ = group_thread->ssd_group_list[1];
	para2.blk_addr_ = blk_addr;
	para2.geo_ = geo;
	para2.buf_g_ = buf_w_2;
	para2.ret_ = ret;

	para3.group_ = group_thread->ssd_group_list[2];
	para3.blk_addr_ = blk_addr;
	para3.geo_ = geo;
	para3.buf_g_ = buf_w_3;
	para3.ret_ = ret;
	
	ret1 = pthread_create(&t1,NULL,ssd_group_write_struct,&(para1));
	if (ret1 != 0)
		printf("write pthread 1 error!\n");
	ret2 = pthread_create(&t2,NULL,ssd_group_write_struct,&(para2));
	if (ret2 != 0)
		printf("write pthread 2 error!\n");
	ret3 = pthread_create(&t3,NULL,ssd_group_write_struct,&(para3));
	if (ret3 != 0)
		printf("write pthread 3 error!\n");
	void *a1, *a2, *a3;
	pthread_join(t1,&a1);
	pthread_join(t2,&a2);
	pthread_join(t3,&a3);
	printf("a1:%d + a2:%d + a3:%d\n",(int *)a1, (int *)a2, (int *)a3);
	free(buf_w_1);
	free(buf_w_2);
	free(buf_w_3);
	if ((int *)a1 > 0 && (int *)a2 > 0 && (int *)a3 > 0)
		return 1;
	return 0;
}

int ssd_group_read_pthread(struct ssd_group_thread *group_thread, struct nvm_addr blk_addr, struct nvm_geo *geo, char *buf_r_g, struct nvm_ret *ret)
{
	pthread_t t1,t2,t3;
	int ret1,ret2,ret3;
	int naddrs = geo->nplanes * geo->nsectors;
	int buf_nbytes = naddrs * geo->sector_nbytes;
	char *buf_r_1 = nvm_buf_alloc(geo, buf_nbytes);
	char *buf_r_2 = nvm_buf_alloc(geo, buf_nbytes);
	char *buf_r_3 = nvm_buf_alloc(geo, buf_nbytes);
	
	memset(buf_r_1, 0 , buf_nbytes);
	memset(buf_r_2, 0 , buf_nbytes);
	memset(buf_r_3, 0 , buf_nbytes);


	struct para_g para1,para2,para3;
	para1.group_ = group_thread->ssd_group_list[0];
	para1.blk_addr_ = blk_addr;
	para1.geo_ = geo;
	para1.buf_g_ = buf_r_1;
	para1.ret_ = ret;
	
	para2.group_ = group_thread->ssd_group_list[1];
	para2.blk_addr_ = blk_addr;
	para2.geo_ = geo;
	para2.buf_g_ = buf_r_2;
	para2.ret_ = ret;

	para3.group_ = group_thread->ssd_group_list[2];
	para3.blk_addr_ = blk_addr;
	para3.geo_ = geo;
	para3.buf_g_ = buf_r_3;
	para3.ret_ = ret;
	
	ret1 = pthread_create(&t1,NULL,ssd_group_read_struct,&(para1));
	if (ret1 != 0)
		printf("read pthread 1 error!\n");
	ret2 = pthread_create(&t2,NULL,ssd_group_read_struct,&(para2));
	if (ret2 != 0)
		printf("read pthread 2 error!\n");
	ret3 = pthread_create(&t3,NULL,ssd_group_read_struct,&(para3));
	if (ret3 != 0)
		printf("read pthread 3 error!\n");
	void *a1, *a2, *a3;
	pthread_join(t1,&a1);
	pthread_join(t2,&a2);
	pthread_join(t3,&a3);
	printf("a1:%d + a2:%d + a3:%d\n",(int *)a1, (int *)a2, (int *)a3);
	if ((int *)a1 > 0 && (int *)a2 > 0 && (int *)a3 > 0){
		for (int i = 0; i < buf_nbytes; ++i) {
			buf_r_g[i*3] = buf_r_1[i];
			buf_r_g[i*3+1] = buf_r_2[i];
			buf_r_g[i*3+2] = buf_r_3[i] ;
		}
		free(buf_r_1);
		free(buf_r_2);
		free(buf_r_3);
		printf ("buf_r_g: \n");
		for( int i = 0; i < 26 * 2; ++i) 
			printf("%c ",buf_r_g[i]);
		printf ("\n");
		return 1;
	}
	return 0;
}
