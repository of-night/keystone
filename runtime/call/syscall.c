//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
#include <sys/select.h>
#include "call/syscall.h"
#include "util/string.h"
#include "edge_call.h"
#include "uaccess.h"
#include "mm/mm.h"
#include "util/rt_util.h"

#include "call/syscall_nums.h"

#ifdef USE_IO_SYSCALL
#include "call/io_wrap.h"
#endif /* USE_IO_SYSCALL */

#ifdef USE_LINUX_SYSCALL
#include "call/linux_wrap.h"
#endif /* USE_LINUX_SYSCALL */

#ifdef USE_NET_SYSCALL
#include "call/net_wrap.h"
#endif /* USE_NET_SYSCALL */

extern void exit_enclave(uintptr_t arg0);

uintptr_t dispatch_edgecall_syscall(struct edge_syscall* syscall_data_ptr, size_t data_len){
  int ret;

  // Syscall data should already be at the edge_call_data section
  /* For now we assume by convention that the start of the buffer is
   * the right place to put calls */
  struct edge_call* edge_call = (struct edge_call*)shared_buffer;

  edge_call->call_id = EDGECALL_SYSCALL;


  if(edge_call_setup_call(edge_call, (void*)syscall_data_ptr, data_len) != 0){
    return -1;
  }

  ret = sbi_stop_enclave(STOP_EDGE_CALL_HOST);

  if (ret != 0) {
    return -1;
  }

  if(edge_call->return_data.call_status != CALL_STATUS_OK){
    return -1;
  }

  uintptr_t return_ptr;
  size_t return_len;
  if(edge_call_ret_ptr(edge_call, &return_ptr, &return_len) != 0){
    return -1;
  }

  if(return_len < sizeof(uintptr_t)){
    return -1;
  }

  return *(uintptr_t*)return_ptr;
}

uintptr_t dispatch_edgecall_ocall( unsigned long call_id,
				   void* data, size_t data_len,
				   void* return_buffer, size_t return_len){

  uintptr_t ret;
  /* For now we assume by convention that the start of the buffer is
   * the right place to put calls */
  struct edge_call* edge_call = (struct edge_call*)shared_buffer;

  /* We encode the call id, copy the argument data into the shared
   * region, calculate the offsets to the argument data, and then
   * dispatch the ocall to host */

  edge_call->call_id = call_id;
  uintptr_t buffer_data_start = edge_call_data_ptr();

  if(data_len > (shared_buffer_size - (buffer_data_start - shared_buffer))){
    goto ocall_error;
  }
  //TODO safety check on source
  copy_from_user((void*)buffer_data_start, (void*)data, data_len);

  if(edge_call_setup_call(edge_call, (void*)buffer_data_start, data_len) != 0){
    goto ocall_error;
  }

  ret = sbi_stop_enclave(STOP_EDGE_CALL_HOST);

  if (ret != 0) {
    goto ocall_error;
  }

  if(edge_call->return_data.call_status != CALL_STATUS_OK){
    goto ocall_error;
  }

  if( return_len == 0 ){
    /* Done, no return */
    return (uintptr_t)NULL;
  }

  uintptr_t return_ptr;
  size_t ret_len_untrusted;
  if(edge_call_ret_ptr(edge_call, &return_ptr, &ret_len_untrusted) != 0){
    goto ocall_error;
  }

  /* Done, there was a return value to copy out of shared mem */
  /* TODO This is currently assuming return_len is the length, not the
     value passed in the edge_call return data. We need to somehow
     validate these. The size in the edge_call return data is larger
     almost certainly.*/
  copy_to_user(return_buffer, (void*)return_ptr, ret_len_untrusted > return_len ? return_len : ret_len_untrusted);

  return 0;

 ocall_error:
  /* TODO In the future, this should fault */
  return 1;
}

uintptr_t handle_copy_from_shared(void* dst, uintptr_t offset, size_t size){

  /* This is where we would handle cache side channels for a given
     platform */

  /* The only safety check we do is to confirm all data comes from the
   * shared region. */
  uintptr_t src_ptr;
  if(edge_call_get_ptr_from_offset(offset, size,
				   &src_ptr) != 0){
    return 1;
  }

  return copy_to_user(dst, (void*)src_ptr, size);
}

// test other enclave access stm
// 不需要从other enclave eapp 中获取数据只需要让other enclave runtime 向stm中写入数据
uintptr_t handle_STM_TEST_OTHER_ENCLAVE_ACCESS() {

  unsigned long long *YXSTM_start_ptr = (unsigned long long *)YXS_trusted_memory;
  // unsigned long long YXSTM_size       = YXS_trusted_memory_size;

  // 本来是有权限的
  memset((void*)YXSTM_start_ptr, 0, 8);
  printf("pmp rwx=111 memset stm successed.\n");

  // 让SM将该enclave对stm的权限由rxw=111 ==> rwx=000
  if (sbi_other_enclave_access_stm_test_set_pmp()) {
    printf("pmp set error\n");
    return 1;
  }
  memset((void*)YXSTM_start_ptr, 0, 8);
  // 如果没出错则会输出下面
  printf("pmp rwx=000 memset stm successed.\n");

  return 0;
}

int map_TEST_OTHER_ENCLAVE_ACCESS_EPM(uintptr_t TEST_ACCESS_EPM_ptr, uintptr_t TEST_ACCESS_EPM_size) {
  uintptr_t va        = EYRIE_TEST_OTHER_ENCLAVE_ACCESS_EPM;
  while (va < EYRIE_TEST_OTHER_ENCLAVE_ACCESS_EPM + TEST_ACCESS_EPM_size) {
    if (!map_page(vpn(va), ppn(TEST_ACCESS_EPM_ptr), PTE_W | PTE_R | PTE_D)) {
      return -1;
    }
    va += RISCV_PAGE_SIZE;
    TEST_ACCESS_EPM_ptr += RISCV_PAGE_SIZE;
  }
  return 0;
}

// test other enclave access epm
// meixiewan
uintptr_t handle_TEST_OTHER_ENCLAVE_ACCESS_EPM_O(void* arg0) {

  // 通过stm接收epm物理地址
  // 
  unsigned long long flag = 0;
  if (copy_from_user((void*)&flag, arg0, sizeof(unsigned long long))) {
    return 1;
  }

  unsigned long long *YXSTM_start_ptr = (unsigned long long *)YXS_trusted_memory;
  unsigned long long YXSTM_size       = YXS_trusted_memory_size;
  unsigned long long MAX_YXSTM_ptr    = (unsigned long long)((unsigned char*)YXSTM_start_ptr + YXSTM_size);

  struct YXSTM_USE {
    unsigned long long *YXSTM_USE_flag_ptr;
    unsigned long long YXSTM_USE_data_offset;
    unsigned long long YXSTM_USE_data_size;
  };

  struct YXSTM_USE YXSTM_use = {
    .YXSTM_USE_flag_ptr = YXSTM_start_ptr,
    .YXSTM_USE_data_offset = 1,
    .YXSTM_USE_data_size = 8
  };
  // YXSTM_use.YXSTM_USE_flag_ptr = YXSTM_start_ptr;
  // YXSTM_use.YXSTM_USE_data_offset = 1;

  uintptr_t epm_base_ptr = 0;

  int i = 0;
  for (i = 0; i < YXSTM_use.YXSTM_USE_data_offset; ++i) {
    if (*(YXSTM_use.YXSTM_USE_flag_ptr + i) == 0) {
      unsigned long long *YXSTM_USE_data_ptr = YXSTM_use.YXSTM_USE_flag_ptr + YXSTM_use.YXSTM_USE_data_offset*2 + i*YXSTM_use.YXSTM_USE_data_size;
      if ((unsigned long long)((unsigned char*)YXSTM_USE_data_ptr + sizeof(test_other_enclave_access_epm)) <= MAX_YXSTM_ptr) {
        epm_base_ptr = *YXSTM_USE_data_ptr;
        *(YXSTM_use.YXSTM_USE_flag_ptr + i) = 0;
        flag = 1;
        if (copy_to_user(arg0, (void*)&flag, sizeof(unsigned long long))) {
          return 1;
        }
      } else {
        printf("YXSTM size too small\n");
      }
      break;
    }
  }

  if (i >= YXSTM_use.YXSTM_USE_data_offset) {
    flag = 0;
    if (copy_to_user(arg0, (void*)&flag, sizeof(unsigned long long))) {
      return 1;
    }
  }

  // 将物理地址映射到EYRIE_TEST_OTHER_ENCLAVE_ACCESS_EPM
  // 默认使用8B大小
  uintptr_t test_other_enclave_access_epm_size = 8;
  //
  map_TEST_OTHER_ENCLAVE_ACCESS_EPM(epm_base_ptr, test_other_enclave_access_epm_size);

  uintptr_t test_other_enclave_access_epm_base_memory = EYRIE_TEST_OTHER_ENCLAVE_ACCESS_EPM;

  // 测试
  memset((void*)test_other_enclave_access_epm_base_memory, 0, test_other_enclave_access_epm_size);
  // 如果没出错则会输出下面
  printf("memset epm successed. test other enclave access epm error! \n");

  return 0;
}

// test other enclave access epm
// meixiewan
uintptr_t handle_TEST_OTHER_ENCLAVE_ACCESS_EPM_S(void* arg0) {

  // 通过stm发送epm物理地址
  // 
  unsigned long long flag = 1;
  if (copy_from_user((void*)&flag, arg0, sizeof(unsigned long long))) {
    return 1;
  }

  unsigned long long *YXSTM_start_ptr = (unsigned long long *)YXS_trusted_memory;
  unsigned long long YXSTM_size       = YXS_trusted_memory_size;
  unsigned long long MAX_YXSTM_ptr    = (unsigned long long)((unsigned char*)YXSTM_start_ptr + YXSTM_size);

  struct YXSTM_USE {
    unsigned long long *YXSTM_USE_flag_ptr;
    unsigned long long YXSTM_USE_data_offset;
    unsigned long long YXSTM_USE_data_size;
  };

  struct YXSTM_USE YXSTM_use = {
    .YXSTM_USE_flag_ptr = YXSTM_start_ptr,
    .YXSTM_USE_data_offset = 1,
    .YXSTM_USE_data_size = 8
  };
  // YXSTM_use.YXSTM_USE_flag_ptr = YXSTM_start_ptr;
  // YXSTM_use.YXSTM_USE_data_offset = 1;

  int i = 0;
  for (i = 0; i < YXSTM_use.YXSTM_USE_data_offset; ++i) {
    if (*(YXSTM_use.YXSTM_USE_flag_ptr + i) == 0) {
      unsigned long long *YXSTM_USE_data_ptr = YXSTM_use.YXSTM_USE_flag_ptr + YXSTM_use.YXSTM_USE_data_offset*2 + i*YXSTM_use.YXSTM_USE_data_size;
      if ((unsigned long long)((unsigned char*)YXSTM_USE_data_ptr + sizeof(test_other_enclave_access_epm)) <= MAX_YXSTM_ptr) {
        *YXSTM_USE_data_ptr = test_other_enclave_access_epm;
        *(YXSTM_use.YXSTM_USE_flag_ptr + i) = 1;
        flag = 0;
        if (copy_to_user(arg0, (void*)&flag, sizeof(unsigned long long))) {
          return 1;
        }
      } else {
        printf("YXSTM size too small\n");
      }
      break;
    }
  }

  if (i >= YXSTM_use.YXSTM_USE_data_offset) {
    flag = 1;
    if (copy_to_user(arg0, (void*)&flag, sizeof(unsigned long long))) {
      return 1;
    }
  }

  // // 默认使用8B大小
  // int test_other_enclave_access_epm_size = 8;

  return 0;
}

uintptr_t handle_get_numberBlock_from_YXSTM(void* dst, uintptr_t get_number, size_t size){

  uintptr_t ret = 0;

  unsigned long long *YXSTM_start_ptr = (unsigned long long *)YXS_trusted_memory;
  unsigned long long YXSTM_size       = YXS_trusted_memory_size;

  // if (sbi_main_enclave_get_numberblock_set_pmp()) {
  //   ret = 1;
  //   goto YXSTM_error;
  // }

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;

  if (number <= 0) {
    ret = 1;
    goto YXSTM_error;
  }

  unsigned long long * number_ptr = YXSTM_start_ptr;
  unsigned long long * length_ptr = YXSTM_start_ptr + number;
  unsigned char * src_ptr         = (unsigned char *)(YXSTM_start_ptr + number + number);

  unsigned long long src_offset = 0;

  int i;
  for (i = 0; i < number; ++i) {
    if (get_number == number_ptr[i]) {
      break;
    }
  }

  unsigned char temp[17] = {0,};

  if (i < number) {
    src_offset = i << 18;
    *((unsigned long long*)temp) = number_ptr[i];
    *(((unsigned long long*)temp) + 1) = length_ptr[i];
    temp[16] = 1;
    if (copy_to_user(dst+17, (void*)(src_ptr + src_offset), length_ptr[i])) {
      ret = 1;
      goto YXSTM_error;
    }
    number_ptr[i] = 0;
    length_ptr[i] = 0;
  } else {
    *((unsigned long long*)temp) = 0;
    *(((unsigned long long*)temp) + 1) = 0;
    temp[16] = 0;
  }
  
  if (copy_to_user(dst, (void*)temp, 17)) {
    ret = 1;
    goto YXSTM_error;
  }

YXSTM_error:
  return ret;
}

uintptr_t handle_set_numberBlock_to_YXSTM(void* src, uintptr_t set_number, size_t size){

  uintptr_t ret = 0;

  // if (sbi_slave_enclave_set_numberblock_set_pmp()) {
  //   ret = 1;
  //   goto YXSTM_error;
  // }

  unsigned long long *YXSTM_start_ptr = (unsigned long long *)YXS_trusted_memory;
  unsigned long long YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;

  if (number <= 0) {
    ret = 1;
    goto YXSTM_error;
  }
  
  unsigned long long * number_ptr = YXSTM_start_ptr;
  unsigned long long * length_ptr = YXSTM_start_ptr + number;
  unsigned char * dst_ptr         = (unsigned char *)(YXSTM_start_ptr + number + number);

  unsigned long long dst_offset = 0;

  int i;
  for (i = 0; i < number; ++i) {
    if ((number_ptr[i] == 0) && (length_ptr[i] == 0)) {
      break;
    }
  }

  unsigned char temp[17] = {0,};

  if (i < number) {
    dst_offset = i << 18;
    if (copy_from_user((void*)(dst_ptr + dst_offset), src+17, size)) {
      ret = 1;
      goto YXSTM_error;
    }
    *(((unsigned long long*)temp) + 1) = length_ptr[i] = size;
    *((unsigned long long*)temp) = number_ptr[i] = set_number;
    temp[16] = 0;
  } else {
    *((unsigned long long*)temp) = 0;
    *(((unsigned long long*)temp) + 1) = 0;
    temp[16] = 1;
  }

  if (copy_to_user(src, (void*)temp, 17)) {
    ret = 1;
    goto YXSTM_error;
  }

YXSTM_error:
  return ret;
}

uintptr_t m_attestt_s_enclave(void *_attested_report) {
  // struct s_attested_report {
  //   unsigned int seq;
  //   unsigned long long nonce;
  //   unsigned char s_hash[64];
  //   unsigned char hmac[64];
  //   unsigned char signature[64];
  // }

  // struct s_attested_report* report = (struct s_attested_report*)_attested_report;
  uint64_t flag = 0;
  if (sbi_m_enclave_attest_s_enclave(_attested_report, kg, &flag)){
    return 1;
  }

  if (flag == 0) {
    printf("m_attest_s %s, flag err ,not match\n", __func__);
  }
  // return memcmp(kg, report->enclave.data, report->enclave.data_len);
  return 0;
}

uintptr_t handle_m_attestt_s_enclave(size_t slave_id, size_t flexible) {
  // 将kg作为 report中的nonce
  // data maxlen = 1024, sizeof(kg)=64

  // printf("[main runtime] %s start 1, slave_id:%d, flexible:%d\n", __func__, slave_id, flexible);

  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 11);

  // printf("[main runtime] %s start 2\n", __func__);

  size_t i = 0;
  while(1) {
    for (i = 0; i < slave_stm_region; i++) {
      // printf("[main runtime] %s start .1.. i:%d, slave_stm_region:%d\n", __func__, i, slave_stm_region);
      if (this_slave_flag_addr[(i*3)] == 1 && this_slave_flag_addr[(i*3) + 1] == 2048) {
        break;
      }
    }

    // printf("[main runtime] %s start .2.. i:%d, slave_stm_region:%d\n", __func__, i, slave_stm_region);

    if (i < slave_stm_region) {
      break;
    }
  }

  // printf("[main runtime] %s start 3\n", __func__);

  memcpy((void*)attested_report, (slave_stm_data_ptr_start + (i << 11)), 2048);
  this_slave_flag_addr[(i*3)] = 0;
  this_slave_flag_addr[(i*3) + 1] = 0;

  // printf("[main runtime] %s start 4\n", __func__);

  // attest s report
  if (m_attestt_s_enclave(attested_report)) {
    printf("attest not match %s, slave_id:%d\n", __func__, slave_id);
    this_slave_flag_addr[(i*3)] = 2;
    return 1;
  }

  this_slave_flag_addr[(i*3)] = 2;
  // printf("[main runtime] %s start 5\n", __func__);

  return 0;
}

uintptr_t handle_s_enclave_attestted(size_t slave_id, size_t flexible) {
  // 将kg作为 report中的nonce
  // data maxlen = 1024, sizeof(kg)=64
  // printf("[slave runtime] %s start 1, slave_id:%d, flexible:%d\n", __func__, slave_id, flexible);
  if (sbi_slave_enclave_attested(attested_report, sbi_random(), kg)) {
    return 1;
  }

  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 11);

  // printf("[slave runtime] %s start 2\n", __func__);
  size_t i = 0;
  while (1) {
    for (i = 0; i < slave_stm_region; i++) {
      // if decrypt, size = 0, 0: then send data
      if (this_slave_flag_addr[(i*3)] == 0 && this_slave_flag_addr[(i*3) + 1] == 0) {
        break;
      }
    }
    if (i < slave_stm_region) {
      break;
    }
  }
  
  // printf("[slave runtime] %s start 3\n", __func__);
  memcpy((slave_stm_data_ptr_start + (i << 11)), (void*)attested_report, 2048);
  this_slave_flag_addr[(i*3)] = 1;
  this_slave_flag_addr[(i*3) + 1] = 2048;

  while(1) {
    if (this_slave_flag_addr[(i*3)] == 2) {
      this_slave_flag_addr[(i*3)] = 0;
      this_slave_flag_addr[(i*3) + 1] = 0;
      memset(slave_stm_data_ptr_start + (i << 11), 0, 2048);
      break;
    }
  }

  // printf("[slave runtime] %s start 4\n", __func__);

  return 0;
}

// slave
// decrypt, size = 1, !0 . slave recv data ==> decrypt = 2
uintptr_t handle_wait_main_dispatch(void* dest, void* block_id, void* block_size, uint64_t slave_id, uint64_t flexible) {
  // printf("[slave runtime] %s start\n", __func__);
  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;
  int ret;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 18);

  size_t i = 0;
  while (1) {
    for (i = 0; i < slave_stm_region; i++) {
      // // if decrypt, size = 1, !0 : then recv data
      // if (this_slave_flag_addr[i*3] == 1 && this_slave_flag_addr[(i*3) + 1] != 0) {
      //   break;
      // }
      // if decrypt, size = 1, !0 : then recv data
      // done care size
      if (this_slave_flag_addr[i*3] == 1) {
        break;
      }
    }
    if (i < slave_stm_region) {
      break;
    }
  }

  ret = copy_to_user(dest, (void*)(slave_stm_data_ptr_start + (i << 18)), this_slave_flag_addr[(i*3)+1]);
  if (ret) {
    return ret;
  }

  ret = copy_to_user(block_size, (void*)(&this_slave_flag_addr[(i*3) + 1]), sizeof(uint64_t));
  if (ret) {
    return ret;
  }

  ret = copy_to_user(block_id, (void*)(&this_slave_flag_addr[(i*3) + 2]), sizeof(uint64_t));
  if (ret) {
    return ret;
  }

  this_slave_flag_addr[(i*3)] = 2;

  if (this_slave_flag_addr[(i*3) + 1] == 0) {
    this_slave_flag_addr[(i*3)] = 0;
    this_slave_flag_addr[(i*3)+2] = 0;
  }

  // printf("[slave runtime] %s, block_id:%d, block_size:%d\n", __func__, this_slave_flag_addr[(i*3) + 2], this_slave_flag_addr[(i*3) + 1]);

  return 0;
}

// main
// decrypt, size = 0, 0 . main send data ==> decrypt, size, id = 1, size, id
uintptr_t handle_main_dispatch_send(void* src, uint64_t block_id, uint64_t block_size, uint64_t slave_id, uint64_t flexible) {
  // printf("[main runtime] %s start\n", __func__);
  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 18);

  size_t i = 0;
  while (1) {
    for (i = 0; i < slave_stm_region; i++) {
      // if decrypt, size = 0, 0: then send data
      if (this_slave_flag_addr[(i*3)] == 0 && this_slave_flag_addr[(i*3) + 1] == 0) {
        break;
      }
    }
    if (i < slave_stm_region) {
      break;
    }
  }

  int ret = copy_from_user((void*)(slave_stm_data_ptr_start + (i << 18)), src, block_size);
  if (ret) {
    return ret;
  }

  // this_slave_flag_addr[(i*3)] = 1;
  // this_slave_flag_addr[(i*3) + 1] = block_size;
  // this_slave_flag_addr[(i*3) + 2] = block_id;

  // dont care size
  this_slave_flag_addr[(i*3) + 1] = block_size;
  this_slave_flag_addr[(i*3) + 2] = block_id;
  this_slave_flag_addr[(i*3)] = 1;
  
  // printf("[main runtime] %s, block_id:%d, block_size:%d\n", __func__, block_id, block_size);

  return 0;
}

// slave
// decrypt, size, id = 2, size, id . slave set data ==> decrypt = 3
uintptr_t handle_slave_set_block(void* src, uint64_t block_id, uint64_t block_size, uint64_t slave_id, uint64_t flexible) {
  // printf("[slave runtime] %s start\n", __func__);
  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 18);

  size_t i = 0;
  while (1) {
    for (i = 0; i < slave_stm_region; i++) {
      if (this_slave_flag_addr[(i*3)] == 2 && this_slave_flag_addr[(i*3) + 1] == block_size && this_slave_flag_addr[(i*3) + 2] == block_id) {
        break;
      }
    }
    if (i < slave_stm_region) {
      break;
    }
  }

  int ret = copy_from_user((void*)(slave_stm_data_ptr_start + (i << 18)), src, block_size);
  if (ret) {
    return ret;
  }

  this_slave_flag_addr[(i*3)] = 3;

  // printf("[slave runtime] %s end\n", __func__);

  return 0;
}

// main
// decrypt, size, id = 3, size, id . main get data ==> decrypt, size, id = 0, 0, 0
uintptr_t handle_get_slave_block(void* dest, uint64_t block_id, uint64_t block_size, uint64_t slave_id, uint64_t flexible) {
  // printf("[main runtime] %s start\n", __func__);
  uint64_t *YXSTM_start_ptr = (uint64_t *)YXS_trusted_memory;
  uint64_t YXSTM_size       = YXS_trusted_memory_size;

  int number = ((YXSTM_size + 0x3ffff) >> 18) - 1;
  if (number <= 0) {
    return 1;
  }

  size_t slave_stm_region = number / (flexible - 1);

  uint64_t *this_slave_flag_addr = YXSTM_start_ptr + (3 * slave_stm_region * (slave_id - 1));
  char *slave_stm_data_ptr_start = (char*)YXSTM_start_ptr + (sizeof(uint64_t) * 3 * slave_stm_region * (flexible - 1)) + ((slave_stm_region * (slave_id - 1)) << 18);

  size_t i = 0;
  while (1) {
    for (i = 0; i < slave_stm_region; i++) {
      if (this_slave_flag_addr[(i*3)] == 3 && this_slave_flag_addr[(i*3) + 1] == block_size && this_slave_flag_addr[(i*3) + 2] == block_id) {
        break;
      }
    }
    if (i < slave_stm_region) {
      break;
    }
  }

  int ret = copy_to_user(dest, (void*)(slave_stm_data_ptr_start + (i << 18)), block_size);
  if (ret) {
    return ret;
  }

  this_slave_flag_addr[(i*3)] = 0;
  this_slave_flag_addr[(i*3)+1] = 0;
  this_slave_flag_addr[(i*3)+2] = 0;

  // printf("[main runtime] %s end\n", __func__);

  return 0;
}

void init_edge_internals(){
  edge_call_init_internals(shared_buffer, shared_buffer_size);
}

void handle_syscall(struct encl_ctx* ctx)
{
  uintptr_t n = ctx->regs.a7;
  uintptr_t arg0 = ctx->regs.a0;
  uintptr_t arg1 = ctx->regs.a1;
  uintptr_t arg2 = ctx->regs.a2;
  uintptr_t arg3 = ctx->regs.a3;
  uintptr_t arg4 = ctx->regs.a4;

  // We only use arg5 in these for now, keep warnings happy.
#if defined(USE_LINUX_SYSCALL) || defined(USE_NET_SYSCALL)
  uintptr_t arg5 = ctx->regs.a5;
#endif /* IO_SYSCALL */
  uintptr_t ret = 0;

  ctx->regs.sepc += 4;

  switch (n) {
  case(RUNTIME_SYSCALL_EXIT):
    sbi_exit_enclave(arg0);
    break;
  case(RUNTIME_SYSCALL_OCALL):
    ret = dispatch_edgecall_ocall(arg0, (void*)arg1, arg2, (void*)arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_SHAREDCOPY):
    ret = handle_copy_from_shared((void*)arg0, arg1, arg2);
    break;
  case(RUNTIME_SYSCALL_CREATE_GROUP):
    copy_from_user((void*)rt_copy_identity, (void*)arg0, arg1);
    ret = sbi_m_enclave_create_group((void*)rt_copy_identity, arg1);
    break;
  case(RUNTIME_SYSCALL_JOIN_GROUP):
    copy_from_user((void*)rt_copy_identity, (void*)arg0, arg1);
    ret = sbi_s_enclave_join_group((void*)rt_copy_identity, arg1);
    //处理返回结果待写
    break;
  case(RUNTIME_SYSCALL_MAIN_ENCLAVE_GET_SLAVE_ENCLAVE_DATA):;
    copy_from_user((void*)rt_copy_slave_data, (void*)arg0, 16);
    ret = sbi_main_enclave_get_slave_enclave_data_yx((void*)rt_copy_slave_data, (void*)(rt_copy_slave_data+17));
    copy_to_user((void*)arg0, (void*)rt_copy_slave_data, 256*1024+8+8+1);
    break;
  case(RUNTIME_SYSCALL_SLAVE_ENCLAVE_SET_DATAPTR):;
    copy_from_user((void*)rt_copy_slave_data, (void*)arg0, 256*1024+8+8+1);
    ret = sbi_slave_enclave_set_dataptr_yx((void*)rt_copy_slave_data, (void*)(rt_copy_slave_data+17));
    copy_to_user((void*)arg0, (void*)rt_copy_slave_data, 8+8+1);
    break;
  case(RUNTIME_SYSCALL_YXSTM_SET_NUMBERBLOCK):;
    ret = handle_set_numberBlock_to_YXSTM((void*)arg0, arg1, arg2);
    break;
  case(RUNTIME_SYSCALL_YXSTM_GET_NUMBERBLOCK):;
    ret = handle_get_numberBlock_from_YXSTM((void*)arg0, arg1, arg2);
    break;
  case(RUNTIME_SYSCALL_STM_ACCESS_TEST):;
    ret = handle_STM_TEST_OTHER_ENCLAVE_ACCESS();
    break;
  case(RUNTIME_SYSCALL_TEST_OTHER_ENCLAVE_ACCESS_EPM_O):;
    ret = handle_TEST_OTHER_ENCLAVE_ACCESS_EPM_O((void*)arg0);
    break;
  case(RUNTIME_SYSCALL_TEST_OTHER_ENCLAVE_ACCESS_EPM_S):;
    ret = handle_TEST_OTHER_ENCLAVE_ACCESS_EPM_S((void*)arg0);
    break;
  case(RUNTIME_SYSCALL_M_ATTEST_S_ENCLAVE):;
    ret = handle_m_attestt_s_enclave(arg0, arg1);
    break;
  case(RUNTIME_SYSCALL_S_ENCLAVE_ATTESTTED):;
    ret = handle_s_enclave_attestted(arg0, arg1);
    break;
  case(RUNTIME_SYSCALL_WAIT_MAIN_DISPATCH):;
    ret = handle_wait_main_dispatch((void*)arg0, (void*)arg1, (void*)arg2, arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_MAIN_DISPATCH_SEND):;
    ret = handle_main_dispatch_send((void*)arg0, arg1, arg2, arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_SLAVE_SET_BLOCK):;
    ret = handle_slave_set_block((void*)arg0, arg1, arg2, arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_GET_SLAVE_BLOCK):;
    ret = handle_get_slave_block((void*)arg0, arg1, arg2, arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_ATTEST_ENCLAVE):;
    copy_from_user((void*)rt_copy_buffer_2, (void*)arg1, arg2);

    ret = sbi_attest_enclave(rt_copy_buffer_1, rt_copy_buffer_2, arg2);

    /* TODO we consistently don't have report size when we need it */
    copy_to_user((void*)arg0, (void*)rt_copy_buffer_1, 2048);
    //print_strace("[ATTEST] p1 0x%p->0x%p p2 0x%p->0x%p sz %lx = %lu\r\n",arg0,arg0_trans,arg1,arg1_trans,arg2,ret);
    break;
  case(RUNTIME_SYSCALL_GET_SEALING_KEY):;
    /* Stores the key receive structure */
    uintptr_t buffer_1_pa = translate((uintptr_t) rt_copy_buffer_1);

    /* Stores the key identifier */
    uintptr_t buffer_2_pa = translate((uintptr_t) rt_copy_buffer_2);

    if (arg1 > sizeof(rt_copy_buffer_1) ||
        arg3 > sizeof(rt_copy_buffer_2)) {
      ret = -1;
      break;
    }

    copy_from_user(rt_copy_buffer_2, (void *)arg2, arg3);

    ret = sbi_get_sealing_key(buffer_1_pa, buffer_2_pa, arg3);

    if (!ret) {
      copy_to_user((void *)arg0, (void *)rt_copy_buffer_1, arg1);
    }

    /* Delete key from copy buffer */
    memset(rt_copy_buffer_1, 0x00, sizeof(rt_copy_buffer_1));

    break;


#ifdef USE_LINUX_SYSCALL
  case(SYS_clock_gettime):
    ret = linux_clock_gettime((__clockid_t)arg0, (struct timespec*)arg1);
    break;

  case(SYS_getrandom):
    ret = linux_getrandom((void*)arg0, (size_t)arg1, (unsigned int)arg2);
    break;

  case(SYS_rt_sigprocmask):
    ret = linux_rt_sigprocmask((int)arg0, (const sigset_t*)arg1, (sigset_t*)arg2);
    break;

  case(SYS_getpid):
    ret = linux_getpid();
    break;

  case(SYS_uname):
    ret = linux_uname((void*) arg0);
    break;

  case(SYS_rt_sigaction):
    ret = linux_RET_ZERO_wrap(n);
    break;

  case(SYS_set_tid_address):
    ret = linux_set_tid_address((int*) arg0);
    break;

  case(SYS_brk):
    ret = syscall_brk((void*) arg0);
    break;

  case(SYS_mmap):
    ret = syscall_mmap((void*) arg0, (size_t)arg1, (int)arg2,
                       (int)arg3, (int)arg4, (__off_t)arg5);
    break;

  case(SYS_munmap):
    ret = syscall_munmap((void*) arg0, (size_t)arg1);
    break;

  case(SYS_mprotect):
    ret = syscall_mprotect((void *) arg0, (size_t) arg1, (int) arg2);
    break;

  case(SYS_exit):
  case(SYS_exit_group):
    print_strace("[runtime] exit or exit_group (%lu)\r\n",n);
    sbi_exit_enclave(arg0);
    break;
#endif /* USE_LINUX_SYSCALL */

#ifdef USE_IO_SYSCALL
  case(SYS_read):
    ret = io_syscall_read((int)arg0, (void*)arg1, (size_t)arg2);
    break;
  case(SYS_write):
    ret = io_syscall_write((int)arg0, (void*)arg1, (size_t)arg2);
    break;
  case(SYS_writev):
    ret = io_syscall_writev((int)arg0, (const struct iovec*)arg1, (int)arg2);
    break;
  case(SYS_readv):
    ret = io_syscall_readv((int)arg0, (const struct iovec*)arg1, (int)arg2);
    break;
  case(SYS_openat):
    ret = io_syscall_openat((int)arg0, (char*)arg1, (int)arg2, (mode_t)arg3);
    break;
  case(SYS_unlinkat):
    ret = io_syscall_unlinkat((int)arg0, (char*)arg1, (int)arg2);
    break;
  case(SYS_fstatat):
    ret = io_syscall_fstatat((int)arg0, (char*)arg1, (struct stat*)arg2, (int)arg3);
    break;
  case(SYS_fstat): 
    ret = io_syscall_fstat((int)arg0, (struct stat*)arg1); 
    break;
  case(SYS_lseek):
    ret = io_syscall_lseek((int)arg0, (off_t)arg1, (int)arg2);
    break;
  case(SYS_ftruncate):
    ret = io_syscall_ftruncate((int)arg0, (off_t)arg1);
    break;
  case(SYS_sync):
    ret = io_syscall_sync();
    break;
  case(SYS_fsync):
    ret = io_syscall_fsync((int)arg0);
    break;
  case(SYS_close):
    ret = io_syscall_close((int)arg0);
    break;
  case(SYS_epoll_create1):
    ret = io_syscall_epoll_create((int) arg0); 
    break;
  case(SYS_epoll_ctl):
    ret = io_syscall_epoll_ctl((int) arg0, (int) arg1, (int) arg2, (uintptr_t) arg3); 
    break;
  case(SYS_epoll_pwait):
    ret = io_syscall_epoll_pwait((int) arg0, (uintptr_t) arg1, (int) arg2, (int) arg3); 
    break;
  case(SYS_fcntl): 
    ret = io_syscall_fcntl((int)arg0, (int)arg1, (uintptr_t)arg2);
    break;
  case(SYS_chdir): 
    ret = io_syscall_chdir((char *) arg0);
    break;
  case(SYS_renameat2): 
    ret = io_syscall_renameat2((int) arg0, (uintptr_t) arg1,  (int) arg2, (uintptr_t) arg3, (int) arg4);
    break;
  case(SYS_umask): 
    ret = io_syscall_umask((int) arg0);
    break;
  case(SYS_getcwd): 
    ret = io_syscall_getcwd((char *)arg0, (size_t)arg1); 
    break;
  case(SYS_pipe2):
    ret = io_syscall_pipe((int*)arg0);
    break;

#endif /* USE_IO_SYSCALL */

#ifdef USE_NET_SYSCALL
  case(SYS_socket):
    ret = io_syscall_socket((int) arg0, (int) arg1, (int) arg2); 
    break; 
  case(SYS_setsockopt):
    ret = io_syscall_setsockopt((int) arg0, (int) arg1, (int) arg2, (int *) arg3, (int) arg4); 
    break; 
  case(SYS_connect):
    ret = io_syscall_connect((int) arg0, (uintptr_t) arg1, (int) arg2);
    break;
  case (SYS_bind):
    ret = io_syscall_bind((int) arg0, (uintptr_t) arg1, (int) arg2);
    break;
  case (SYS_listen):
    ret = io_syscall_listen((int) arg0, (uintptr_t) arg1);
    break;
  case (SYS_accept):
    ret = io_syscall_accept((int) arg0, (uintptr_t) arg1, (uintptr_t) arg2);
    break;
  case(SYS_recvfrom):
    ret = io_syscall_recvfrom((int) arg0, (uintptr_t) arg1, (int) arg2, (int) arg3, (uintptr_t) arg4, (uintptr_t) arg5);
    break;
  case(SYS_sendto):
    ret = io_syscall_sendto((int) arg0, (uintptr_t) arg1, (int) arg2, (int) arg3, (uintptr_t) arg4, (int) arg5);
    break;
  case(SYS_sendfile):
    ret = io_syscall_sendfile((int) arg0, (int) arg1, (uintptr_t) arg2, (int) arg3);
    break;
  case(SYS_getpeername): 
    ret = io_syscall_getpeername((int) arg0,  (uintptr_t) arg1, (uintptr_t) arg2);
    break;
  case(SYS_getsockname): 
    ret = io_syscall_getsockname((int) arg0,  (uintptr_t) arg1, (uintptr_t) arg2);
    break;
  case(SYS_getuid): 
    ret = io_syscall_getuid(); 
    break; 
  case(SYS_pselect6): 
    ret = io_syscall_pselect((int) arg0, (uintptr_t) arg1, (uintptr_t) arg2, (uintptr_t) arg3, (uintptr_t) arg4, (uintptr_t) arg5);
    break;
#endif /* USE_NET_SYSCALL */


  case(RUNTIME_SYSCALL_UNKNOWN):
  default:
    print_strace("[runtime] syscall %ld not implemented\r\n", (unsigned long) n);
    ret = -1;
    break;
  }

  /* store the result in the stack */
  ctx->regs.a0 = ret;
  return;
}
