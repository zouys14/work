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
  free(group);
}

{
  int naddrs = geo->nplanes * geo->nsectors;
  int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
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
    if (res < 0){
      printf("erase %s error!\n", nvm_dev_get_name(group->ssd_list[i]));
      return 0;
    }
  }
  return 1;
}

{
  int naddrs = geo->nplanes * geo->nsectors;
  int use_meta = 0;
  int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
  struct nvm_addr addrs[naddrs];
  char *buf_w = NULL, *meta_w = NULL;

  int buf_nbytes = naddrs * geo->sector_nbytes;
  int meta_nbytes = naddrs * geo->meta_nbytes;

  buf_w = nvm_buf_alloc(geo, buf_nbytes);
  if (!buf_w) {
    printf("alloc buf_w error!\n");
    return 0;
  }
  nvm_buf_fill(buf_w, buf_nbytes);

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
    if (strlen(meta_descr) > geo->meta_nbytes) {
    }

    memcpy(meta_w + i * geo->meta_nbytes, meta_descr,
           strlen(meta_descr));
  }

  for (int pg = 0; pg < geo->npages; ++pg) {

      addrs[i].g.pg = pg;
      addrs[i].g.sec = i % geo->nsectors;
      addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
    }

    int num = group->ssd_number;
    int res = -1;
    for (int j = 0; j < num; j++){
      if (res < 0) {
        return 0;
      }
    }
  }
  return 1;
}

{
  int naddrs = geo->nplanes * geo->nsectors;
  int use_meta = 0;
  int pmode = nvm_dev_get_pmode(group->ssd_list[0]);
  struct nvm_addr addrs[naddrs];
  char *buf_r = NULL, *meta_r = NULL;

  int buf_nbytes = naddrs * geo->sector_nbytes;
  int meta_nbytes = naddrs * geo->meta_nbytes;

  buf_r = nvm_buf_alloc(geo, buf_nbytes);
  if (!buf_r) {
    printf("alloc buf_r error!\n");
    return 0;
  meta_r = nvm_buf_alloc(geo, meta_nbytes);
    return 0;
  }

  for (int pg = 0; pg < geo->npages; ++pg) {
    for (int i = 0; i < naddrs; ++i) {
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
      res = nvm_addr_read(group->ssd_list[j], addrs, naddrs, buf_r, use_meta ? mm
eta_r : NULL, pmode, ret);
      if (res < 0 && j < num-1)
        printf("read page %d error in dev %s!\n", pg, nvm_dev_get_name(group->sss
d_list[j]));
      else if (res < 0 && j == num-1){
        printf("read page %d error in all dev!\n", pg);
        return 0;
      }
      else if (res > 0){
        printf("read data sucessful in dev %s!\n",nvm_dev_get_name(group->ssd_lii
st[j]));
        break;
      }
    }
  }
  return 1;
}

