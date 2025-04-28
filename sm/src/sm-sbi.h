//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef _KEYSTONE_SBI_H_
#define _KEYSTONE_SBI_H_

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>

unsigned long
sbi_sm_create_enclave(unsigned long *out_val, uintptr_t create_args);

unsigned long
sbi_sm_destroy_enclave(unsigned long eid);

unsigned long
sbi_sm_run_enclave(struct sbi_trap_regs *regs, unsigned long eid);

unsigned long
sbi_sm_exit_enclave(struct sbi_trap_regs *regs, unsigned long retval);

unsigned long
sbi_sm_stop_enclave(struct sbi_trap_regs *regs, unsigned long request);

unsigned long
sbi_sm_resume_enclave(struct sbi_trap_regs *regs, unsigned long eid);

unsigned long
sbi_sm_attest_enclave(uintptr_t report, uintptr_t data, uintptr_t size);

unsigned long
sbi_sm_m_enclave_create_group(uintptr_t identity, uintptr_t size);

unsigned long
sbi_sm_s_enclave_join_group(uintptr_t identity, uintptr_t size);

unsigned long
sbi_sm_main_enclave_get_slave_enclave_data(uintptr_t dest, uintptr_t size, uintptr_t numbers);

unsigned long
sbi_sm_slave_enclave_set_dataptr(uintptr_t src, uintptr_t size, uintptr_t numbers);

unsigned long
sbi_sm_main_enclave_get_slave_enclave_data_yx(uintptr_t temp_ptr, uintptr_t dest_ptr);

unsigned long
sbi_sm_slave_enclave_set_dataptr_yx(uintptr_t temp_ptr, uintptr_t data_ptr);

unsigned long
sbi_sm_main_enclave_get_numberblock_set_pmp();

unsigned long
sbi_sm_slave_enclave_set_numberblock_set_pmp();

unsigned long
sbi_sm_other_enclave_access_stm_test_set_pmp();

unsigned long
sbi_sm_get_sealing_key(uintptr_t seal_key, uintptr_t key_ident, size_t key_ident_size);

unsigned long
sbi_sm_random(void);

unsigned long
sbi_sm_call_plugin(uintptr_t plugin_id, uintptr_t call_id, uintptr_t arg0, uintptr_t arg1);

#endif
