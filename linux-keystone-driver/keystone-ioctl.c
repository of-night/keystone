//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "keystone.h"
#include "keystone-sbi.h"
#include "keystone_user.h"
#include <asm/sbi.h>
#include <linux/uaccess.h>
#include <linux/string.h>

extern spinlock_t YXSTM_spinlock;

static unsigned long testaccess_other_os_access_epm = -1UL;

int __keystone_destroy_enclave(unsigned int ueid);

int keystone_create_enclave(struct file *filep, unsigned long arg)
{
  /* create parameters */
  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;

  struct enclave *enclave;
  enclave = create_enclave(enclp->min_pages);

  if (enclave == NULL) {
    return -ENOMEM;
  }

  /* Pass base page table */
  enclp->epm_paddr = enclave->epm->pa;
  enclp->epm_size = enclave->epm->size;

  /* allocate UID */
  enclp->eid = enclave_idr_alloc(enclave);

  filep->private_data = (void *) enclp->eid;

  return 0;
}

extern struct GLOBAL_YXSTM global_yxstm;

int YXSTM_init_ioctl(struct file *filep, unsigned long arg)
{
  int ret = 0;
  struct YXSTM *YXSTM;
  struct enclave *enclave;
  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;
  long long unsigned YXSTrusted_size = enclp->YXSTM_size;
  long long unsigned ms_YXSTM = enclp->ms;

  enclave = get_enclave_by_id(enclp->eid);

  if(!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if(!ms_YXSTM) {
    keystone_err("%s, error ms_YXSTM\n", __func__);
    return -EINVAL;
  }

  YXSTM = kmalloc(sizeof(struct YXSTM), GFP_KERNEL);
  if (!YXSTM) {
    ret = -ENOMEM;
    return ret;
  }

  spin_lock(&YXSTM_spinlock); // 获取锁
  if(global_yxstm.globalCount == 0) {
    global_yxstm.globalCount++;
    ret = YXSTM_init(&global_yxstm.global_yxstm, YXSTrusted_size);
    // keystone_info("YXSTM driver %s testing, ptr=%lu, size2=%lu, size3=%lu\n", __func__, global_yxstm.global_yxstm.ptr, global_yxstm.global_yxstm.size, YXSTrusted_size);
  }

  YXSTM->order            = global_yxstm.global_yxstm.order;
  YXSTM->ptr              = global_yxstm.global_yxstm.ptr;
  YXSTM->root_page_table  = global_yxstm.global_yxstm.root_page_table;
  YXSTM->size             = global_yxstm.global_yxstm.size;

  spin_unlock(&YXSTM_spinlock); // 释放锁

  // keystone_info("YXSTM driver %s testing, ptr=%lu, size1=%lu, size2=%lu, size3=%lu\n", __func__, global_yxstm.global_yxstm.ptr, YXSTM->size, global_yxstm.global_yxstm.size, YXSTrusted_size);

  enclave->ms = ms_YXSTM;
  enclave->YXSTM = YXSTM;

  enclp->YXSTM_paddr = __pa(YXSTM->ptr);

  return ret;

}

int keystone_finalize_enclave(unsigned long arg)
{
  struct sbiret ret;
  struct enclave *enclave;
  struct utm *utm;
  struct YXSTM *YXSTM;
  struct keystone_sbi_create_t create_args;

  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;

  enclave = get_enclave_by_id(enclp->eid);
  if(!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  enclave->is_init = false;

  /* SBI Call */
  create_args.epm_region.paddr = enclave->epm->pa;
  create_args.epm_region.size = enclave->epm->size;

  utm = enclave->utm;

  if (utm) {
    create_args.utm_region.paddr = __pa(utm->ptr);
    create_args.utm_region.size = utm->size;
  } else {
    create_args.utm_region.paddr = 0;
    create_args.utm_region.size = 0;
  }

  YXSTM = enclave->YXSTM;

  if (YXSTM) {
    create_args.YXSTM_region.paddr = __pa(YXSTM->ptr);
    create_args.YXSTM_region.size = YXSTM->size;

    if(enclave->ms==0){
      keystone_err("%s, error enclp->ms\n", __func__);
      goto error_destroy_enclave;
    }

  } else {
    create_args.YXSTM_region.paddr = 0;
    create_args.YXSTM_region.size = 0;
    if(enclave->ms!=0){
      keystone_err("%s, error enclp->ms\n", __func__);
      goto error_destroy_enclave;
    }
  }

  // physical addresses for runtime, user, and freemem
  create_args.runtime_paddr = enclp->runtime_paddr;
  create_args.user_paddr = enclp->user_paddr;
  create_args.free_paddr = enclp->free_paddr;
  create_args.free_requested = enclp->free_requested;
  create_args.ms_YXSTM = enclave->ms;

  ret = sbi_sm_create_enclave(&create_args);

  if (ret.error) {
    keystone_err("keystone_create_enclave: SBI call failed with error code %ld\n", ret.error);
    goto error_destroy_enclave;
  }

  // char testaccess[4*1024] = {0,};
  // memcpy((void*)testaccess, (void*)enclave->epm->ptr, 4*1024);
  // keystone_info("test os access epm fault! memcpy errors test %s\n",__func__);
  // memset((void*)enclave->epm->ptr, 0, 4 * 1024);
  // keystone_info("test os access epm fault! errors test %s\n",__func__);

  enclave->eid = ret.value;

  return 0;

error_destroy_enclave:
  /* This can handle partial initialization failure */
  destroy_enclave(enclave);

  return -EINVAL;

}

int keystone_test_other_os_access_epm_create_enclave(struct file *filep, unsigned long arg)
{
  /* create parameters */
  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;

  struct enclave *enclave;
  enclave = create_enclave(enclp->min_pages);

  if (enclave == NULL) {
    return -ENOMEM;
  }

  /* Pass base page table */
  enclp->epm_paddr = enclave->epm->pa;
  enclp->epm_size = enclave->epm->size;

  /* allocate UID */
  enclp->eid = enclave_idr_alloc(enclave);

  spin_lock(&YXSTM_spinlock);
  if (testaccess_other_os_access_epm == -1UL) {
    testaccess_other_os_access_epm = enclp->eid;
  }
  spin_unlock(&YXSTM_spinlock);

  filep->private_data = (void *) enclp->eid;

  return 0;
}

int keystone_test_other_os_access_epm_finalize_enclave(unsigned long arg)
{
  struct sbiret ret;
  struct enclave *enclave;
  struct utm *utm;
  struct YXSTM *YXSTM;
  struct keystone_sbi_create_t create_args;

  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;

  enclave = get_enclave_by_id(enclp->eid);
  if(!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  enclave->is_init = false;

  /* SBI Call */
  create_args.epm_region.paddr = enclave->epm->pa;
  create_args.epm_region.size = enclave->epm->size;

  utm = enclave->utm;

  if (utm) {
    create_args.utm_region.paddr = __pa(utm->ptr);
    create_args.utm_region.size = utm->size;
  } else {
    create_args.utm_region.paddr = 0;
    create_args.utm_region.size = 0;
  }

  YXSTM = enclave->YXSTM;

  if (YXSTM) {
    create_args.YXSTM_region.paddr = __pa(YXSTM->ptr);
    create_args.YXSTM_region.size = YXSTM->size;

    if(enclave->ms==0){
      keystone_err("%s, error enclp->ms\n", __func__);
      goto error_destroy_enclave;
    }

  } else {
    create_args.YXSTM_region.paddr = 0;
    create_args.YXSTM_region.size = 0;
    if(enclave->ms!=0){
      keystone_err("%s, error enclp->ms\n", __func__);
      goto error_destroy_enclave;
    }
  }

  // physical addresses for runtime, user, and freemem
  create_args.runtime_paddr = enclp->runtime_paddr;
  create_args.user_paddr = enclp->user_paddr;
  create_args.free_paddr = enclp->free_paddr;
  create_args.free_requested = enclp->free_requested;
  create_args.ms_YXSTM = enclave->ms;

  ret = sbi_sm_create_enclave(&create_args);

  spin_lock(&YXSTM_spinlock);
  struct enclave *testaccess_other_os_access_enclave = NULL;
  if (testaccess_other_os_access_epm != enclp->eid && testaccess_other_os_access_epm != -1UL) {
    testaccess_other_os_access_enclave = get_enclave_by_id(testaccess_other_os_access_epm);
  }
  spin_unlock(&YXSTM_spinlock);

  keystone_info("testaccess_other_os_access_epm:%ul, eid:%lu func:%s\n",testaccess_other_os_access_epm, enclp->eid, __func__);

  if (testaccess_other_os_access_enclave) {
    char testaccess[4*1024] = {0,};
    memcpy((void*)testaccess, (void*)testaccess_other_os_access_enclave->epm->ptr, 4*1024);
    // ruguokeyifangwenzehuishuchuxiamiandeyuju
    keystone_info("other_os_access_epm fault! memcpy test error, func:%s, cpy_dst_data:%02x\n", __func__, testaccess[5]);
		keystone_info("other_os_access_epm fault! memcpy test error, func:%s, cpy_src_data:%02x\n", __func__, *(((char *)testaccess_other_os_access_enclave->epm->ptr) + 5));
    memset((void*)testaccess_other_os_access_enclave->epm->ptr, 0, 4*1024);
    keystone_info("other_os_access_epm fault! memset test error, func:%s\n", __func__);
  }

  if (ret.error) {
    keystone_err("keystone_create_enclave: SBI call failed with error code %ld\n", ret.error);
    goto error_destroy_enclave;
  }

  enclave->eid = ret.value;

  return 0;

error_destroy_enclave:
  /* This can handle partial initialization failure */
  destroy_enclave(enclave);

  return -EINVAL;

}

int keystone_run_enclave(unsigned long data)
{
  struct sbiret ret;
  unsigned long ueid;
  struct enclave* enclave;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;

  ueid = arg->eid;
  enclave = get_enclave_by_id(ueid);

  if (!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_run_enclave(enclave->eid);

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

int utm_init_ioctl(struct file *filp, unsigned long arg)
{
  int ret = 0;
  struct utm *utm;
  struct enclave *enclave;
  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;
  long long unsigned untrusted_size = enclp->utm_size;

  enclave = get_enclave_by_id(enclp->eid);

  if(!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  utm = kmalloc(sizeof(struct utm), GFP_KERNEL);
  if (!utm) {
    ret = -ENOMEM;
    return ret;
  }

  ret = utm_init(utm, untrusted_size);

  /* prepare for mmap */
  enclave->utm = utm;

  enclp->utm_paddr = __pa(utm->ptr);

  return ret;
}


int keystone_destroy_enclave(struct file *filep, unsigned long arg)
{
  int ret;
  struct keystone_ioctl_create_enclave *enclp = (struct keystone_ioctl_create_enclave *) arg;
  unsigned long ueid = enclp->eid;

  ret = __keystone_destroy_enclave(ueid);
  if (!ret) {
    filep->private_data = NULL;
  }
  return ret;
}

int __keystone_destroy_enclave(unsigned int ueid)
{
  struct sbiret ret;
  struct enclave *enclave;
  enclave = get_enclave_by_id(ueid);

  if (!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid >= 0) {
    ret = sbi_sm_destroy_enclave(enclave->eid);
    if (ret.error) {
      keystone_err("fatal: cannot destroy enclave: SBI failed with error code %ld\n", ret.error);
      return -EINVAL;
    }
  } else {
    keystone_warn("keystone_destroy_enclave: skipping (enclave does not exist)\n");
  }


  destroy_enclave(enclave);
  enclave_idr_remove(ueid);

  return 0;
}

int keystone_resume_enclave(unsigned long data)
{
  struct sbiret ret;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;
  unsigned long ueid = arg->eid;
  struct enclave* enclave;
  enclave = get_enclave_by_id(ueid);

  if (!enclave)
  {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_resume_enclave(enclave->eid);

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

int keystone_test_os_access_stm_run_enclave(unsigned long data)
{
  struct sbiret ret;
  unsigned long ueid;
  struct enclave* enclave;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;

  ueid = arg->eid;
  enclave = get_enclave_by_id(ueid);

  if (!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_run_enclave(enclave->eid);

  struct YXSTM *YXSTM;
  YXSTM = enclave->YXSTM;

  char testaccess[4*1024] = {0,};

  if (YXSTM) {
    if(enclave->ms==0){
      keystone_err("%s, error enclp->ms\n", __func__);
      keystone_err("no ms, cant test os_access_stm, func:%s\n", __func__);
    }
    memcpy((void*)testaccess, (void*)YXSTM->ptr, 4*1024);
    // ruguokeyifangwenzehuishuchuxiamiandeyuju
		keystone_info("os_access_stm fault! memcpy test error, func:%s, cpy_dst_data:%02x\n", __func__, testaccess[5]);
		keystone_info("os_access_stm fault! memcpy test error, func:%s, cpy_src_data:%02x\n", __func__, *(((char *)YXSTM->ptr) + 5));
    memset((void*)YXSTM->ptr, 0, 4*1024);
    keystone_info("os_access_stm fault! memset test error, func:%s\n", __func__);

  } else {
    keystone_err("dont allocated stm, func:%s\n", __func__);
    if(enclave->ms!=0){
      keystone_err("%s, error enclp->ms\n", __func__);
    }
  }

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

int keystone_test_os_access_stm_resume_enclave(unsigned long data)
{
  struct sbiret ret;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;
  unsigned long ueid = arg->eid;
  struct enclave* enclave;
  enclave = get_enclave_by_id(ueid);

  if (!enclave)
  {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_resume_enclave(enclave->eid);

  struct YXSTM *YXSTM;
  YXSTM = enclave->YXSTM;

  char testaccess[4*1024] = {0,};

  if (YXSTM) {
    if(enclave->ms==0){
      keystone_err("%s, error enclp->ms\n", __func__);
      keystone_err("no ms, cant test os_access_stm, func:%s\n", __func__);
    }
    memcpy((void*)testaccess, (void*)YXSTM->ptr, 4*1024);
    // ruguokeyifangwenzehuishuchuxiamiandeyuju
		keystone_info("os_access_stm fault! memcpy test error, func:%s, cpy_dst_data:%02x\n", __func__, testaccess[5]);
		keystone_info("os_access_stm fault! memcpy test error, func:%s, cpy_src_data:%02x\n", __func__, *(((char *)YXSTM->ptr) + 5));
    memset((void*)YXSTM->ptr, 0, 4*1024);
    keystone_info("os_access_stm fault! memset test error, func:%s\n", __func__);

  } else {
    keystone_err("dont allocated stm, func:%s\n", __func__);
    if(enclave->ms!=0){
      keystone_err("%s, error enclp->ms\n", __func__);
    }
  }

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

int keystone_test_os_access_epm_run_enclave(unsigned long data)
{
  struct sbiret ret;
  unsigned long ueid;
  struct enclave* enclave;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;

  ueid = arg->eid;
  enclave = get_enclave_by_id(ueid);

  if (!enclave) {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_run_enclave(enclave->eid);

  struct epm *epm;
  epm = enclave->epm;

  char testaccess[4*1024] = {0,};

  memcpy((void*)testaccess, (void*)epm->ptr, 4*1024);
  // ruguokeyifangwenzehuishuchuxiamiandeyuju
	keystone_info("os_access_epm fault! memcpy test error, func:%s, cpy_dst_data:%02x\n", __func__, testaccess[5]);
	keystone_info("os_access_epm fault! memcpy test error, func:%s, cpy_src_data:%02x\n", __func__, *(((char *)epm->ptr) + 5));
  memset((void*)epm->ptr, 0, 4*1024);
  keystone_info("os_access_epm fault! memset test error, func:%s\n", __func__);

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

int keystone_test_os_access_epm_resume_enclave(unsigned long data)
{
  struct sbiret ret;
  struct keystone_ioctl_run_enclave *arg = (struct keystone_ioctl_run_enclave*) data;
  unsigned long ueid = arg->eid;
  struct enclave* enclave;
  enclave = get_enclave_by_id(ueid);

  if (!enclave)
  {
    keystone_err("invalid enclave id\n");
    return -EINVAL;
  }

  if (enclave->eid < 0) {
    keystone_err("real enclave does not exist\n");
    return -EINVAL;
  }

  ret = sbi_sm_resume_enclave(enclave->eid);

  struct epm *epm;
  epm = enclave->epm;

  char testaccess[4*1024] = {0,};

  memcpy((void*)testaccess, (void*)epm->ptr, 4*1024);
  // ruguokeyifangwenzehuishuchuxiamiandeyuju
	keystone_info("os_access_epm fault! memcpy test error, func:%s, cpy_dst_data:%02x\n", __func__, testaccess[5]);
	keystone_info("os_access_epm fault! memcpy test error, func:%s, cpy_src_data:%02x\n", __func__, *(((char *)epm->ptr) + 5));
  memset((void*)epm->ptr, 0, 4*1024);
  keystone_info("os_access_epm fault! memset test error, func:%s\n", __func__);

  arg->error = ret.error;
  arg->value = ret.value;

  return 0;
}

long keystone_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
  long ret;
  char data[512];

  size_t ioc_size;

  if (!arg)
    return -EINVAL;

  ioc_size = _IOC_SIZE(cmd);
  ioc_size = ioc_size > sizeof(data) ? sizeof(data) : ioc_size;

  if (copy_from_user(data,(void __user *) arg, ioc_size))
    return -EFAULT;

  switch (cmd) {
    case KEYSTONE_IOC_CREATE_ENCLAVE:
      ret = keystone_create_enclave(filep, (unsigned long) data);
      break;
    case KEYSTONE_IOC_FINALIZE_ENCLAVE:
      ret = keystone_finalize_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_DESTROY_ENCLAVE:
      ret = keystone_destroy_enclave(filep, (unsigned long) data);
      break;
    case KEYSTONE_IOC_RUN_ENCLAVE:
      ret = keystone_run_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_RESUME_ENCLAVE:
      ret = keystone_resume_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OS_ACCESS_STM_RUN_ENCLAVE:
      ret = keystone_test_os_access_stm_run_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OS_ACCESS_STM_RESUME_ENCLAVE:
      ret = keystone_test_os_access_stm_resume_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OS_ACCESS_EPM_RUN_ENCLAVE:
      ret = keystone_test_os_access_epm_run_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OS_ACCESS_EPM_RESUME_ENCLAVE:
      ret = keystone_test_os_access_epm_resume_enclave((unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OTHER_OS_ACCESS_EPM_CREATE_ENCLAVE:
      ret = keystone_test_other_os_access_epm_create_enclave(filep, (unsigned long) data);
      break;
    case KEYSTONE_IOC_TEST_OTHER_OS_ACCESS_EPM_FINALIZE_ENCLAVE:
      ret = keystone_test_other_os_access_epm_finalize_enclave((unsigned long) data);
      break;
    /* Note that following commands could have been implemented as a part of ADD_PAGE ioctl.
     * However, there was a weird bug in compiler that generates a wrong control flow
     * that ends up with an illegal instruction if we combine switch-case and if statements.
     * We didn't identified the exact problem, so we'll have these until we figure out */
    case KEYSTONE_IOC_UTM_INIT:
      ret = utm_init_ioctl(filep, (unsigned long) data);
      break;
    case KEYSTONE_IOC_YXSTM_INIT:
      ret = YXSTM_init_ioctl(filep, (unsigned long) data);
      break;
    default:
      return -ENOSYS;
  }

  if (copy_to_user((void __user*) arg, data, ioc_size))
    return -EFAULT;

  return ret;
}

int keystone_release(struct inode *inode, struct file *file) {
  unsigned long ueid = (unsigned long)(file->private_data);
  struct enclave *enclave;

  /* enclave has been already destroyed */
  if (!ueid) {
    return 0;
  }

  /* We need to send destroy enclave just the eid to close. */
  enclave = get_enclave_by_id(ueid);

  if (!enclave) {
    /* If eid is set to the invalid id, then we do not do anything. */
    return -EINVAL;
  }
  if (enclave->close_on_pexit) {
    return __keystone_destroy_enclave(ueid);
  }
  return 0;
}
