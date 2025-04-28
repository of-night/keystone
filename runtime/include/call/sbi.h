//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef __SBI_H_
#define __SBI_H_

#include <stdint.h>
#include <stddef.h>

#include "sm_call.h"

void
sbi_putchar(char c);
void
sbi_set_timer(uint64_t stime_value);
uintptr_t
sbi_stop_enclave(uint64_t request);
void
sbi_exit_enclave(uint64_t retval);
uintptr_t
sbi_random();
uintptr_t
sbi_query_multimem(size_t *size);
uintptr_t
sbi_query_multimem_addr(uintptr_t *addr);
uintptr_t
sbi_attest_enclave(void* report, void* buf, uintptr_t len);
uintptr_t
sbi_get_sealing_key(uintptr_t key_struct, uintptr_t key_ident, uintptr_t len);
uintptr_t
sbi_m_enclave_create_group(void* identity, uintptr_t size);
uintptr_t
sbi_s_enclave_join_group(void* identity, uintptr_t size);
uintptr_t
sbi_main_enclave_get_slave_enclave_data(void* dest, uintptr_t size, uintptr_t numbers);
uintptr_t
sbi_slave_enclave_set_dataptr(void* src, uintptr_t size, uintptr_t numbers);
uintptr_t
sbi_main_enclave_get_slave_enclave_data_yx(void* state, void* dest);
uintptr_t
sbi_slave_enclave_set_dataptr_yx(void* state, void* src);
uintptr_t
sbi_slave_enclave_set_numberblock_set_pmp();
uintptr_t
sbi_main_enclave_get_numberblock_set_pmp();
uintptr_t
sbi_other_enclave_access_stm_test_set_pmp();

#endif
