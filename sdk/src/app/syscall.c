//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "syscall.h"

/* this implementes basic system calls for the enclave */

int
ocall(
    unsigned long call_id, void* data, size_t data_len, void* return_buffer,
    size_t return_len) {
  return SYSCALL_5(RUNTIME_SYSCALL_OCALL,
      call_id, data, data_len, return_buffer, return_len);
}

int
copy_from_shared(void* dst, uintptr_t offset, size_t data_len) {
  return SYSCALL_3(RUNTIME_SYSCALL_SHAREDCOPY, dst, offset, data_len);
}

int
attest_enclave(void* report, void* data, size_t size) {
  return SYSCALL_3(RUNTIME_SYSCALL_ATTEST_ENCLAVE, report, data, size);
}

int
m_enclave_create_group(void* identity, size_t size) {
  return SYSCALL_2(RUNTIME_SYSCALL_CREATE_GROUP, identity, size);
}

int
s_enclave_join_group(void* identity, size_t size) {
  return SYSCALL_2(RUNTIME_SYSCALL_JOIN_GROUP, identity, size);
}

int
slave_enclave_set_dataptr(void* src, size_t size, size_t numbers) {
  return SYSCALL_3(RUNTIME_SYSCALL_SLAVE_ENCLAVE_SET_DATAPTR, src, size, numbers);
}

int
main_enclave_get_slave_enclave_data(void* dest, size_t size, size_t numbers) {
  return SYSCALL_3(RUNTIME_SYSCALL_MAIN_ENCLAVE_GET_SLAVE_ENCLAVE_DATA, dest, size, numbers);
}

int
slave_enclave_set_dataptr_yx(void* src) {
  return SYSCALL_1(RUNTIME_SYSCALL_SLAVE_ENCLAVE_SET_DATAPTR, src);
}

int
main_enclave_get_slave_enclave_data_yx(void* dest) {
  return SYSCALL_1(RUNTIME_SYSCALL_MAIN_ENCLAVE_GET_SLAVE_ENCLAVE_DATA, dest);
}

int
slave_enclave_set_numberblock(void* src, size_t set_number, size_t size) {
  return SYSCALL_3(RUNTIME_SYSCALL_YXSTM_SET_NUMBERBLOCK, src, set_number, size);
}

int
main_enclave_get_numberblock(void* dest, size_t set_number, size_t size) {
  return SYSCALL_3(RUNTIME_SYSCALL_YXSTM_GET_NUMBERBLOCK, dest, set_number, size);
}

int
other_enclave_access_stm_test() {
  return SYSCALL_0(RUNTIME_SYSCALL_STM_ACCESS_TEST);
}

int
other_enclave_access_epm_test_o(void* flag) {
  return SYSCALL_1(RUNTIME_SYSCALL_TEST_OTHER_ENCLAVE_ACCESS_EPM_O, flag);
}

int
other_enclave_access_epm_test_s(void* flag) {
  return SYSCALL_1(RUNTIME_SYSCALL_TEST_OTHER_ENCLAVE_ACCESS_EPM_S, flag);
}

int
m_attestt_s_enclave(){
  return SYSCALL_0(RUNTIME_SYSCALL_M_ATTEST_S_ENCLAVE);
}

int
s_enclave_attestted(){
  return SYSCALL_0(RUNTIME_SYSCALL_S_ENCLAVE_ATTESTTED);
}

int
wait_main_dispatch(void* dest, void* block_id, void* block_size, size_t slave_id, size_t flexible) {
  return SYSCALL_5(RUNTIME_SYSCALL_WAIT_MAIN_DISPATCH, dest, block_id, block_size, slave_id, flexible);
}

int
main_dispatch_send(void* src, size_t block_id, size_t block_size, size_t slave_id, size_t flexible) {
  return SYSCALL_5(RUNTIME_SYSCALL_MAIN_DISPATCH_SEND, src, block_id, block_size, slave_id, flexible);
}

int
slave_set_block(void* src, size_t block_id, size_t block_size, size_t slave_id, size_t flexible) {
  return SYSCALL_5(RUNTIME_SYSCALL_SLAVE_SET_BLOCK, src, block_id, block_size, slave_id, flexible);
}

int
get_slave_block(void* dest, size_t block_id, size_t block_size, size_t slave_id, size_t flexible) {
  return SYSCALL_5(RUNTIME_SYSCALL_GET_SLAVE_BLOCK, dest, block_id, block_size, slave_id, flexible);
}

/* returns sealing key */
int
get_sealing_key(
    struct sealing_key* sealing_key_struct, size_t sealing_key_struct_size,
    void* key_ident, size_t key_ident_size) {
  return SYSCALL_4(RUNTIME_SYSCALL_GET_SEALING_KEY,
      sealing_key_struct, sealing_key_struct_size,
      key_ident, key_ident_size);
}
