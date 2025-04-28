//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#ifndef TARGET_PLATFORM_HEADER
#error "SM requires a defined platform to build"
#endif

#include "sm.h"
#include "pmp.h"
#include "thread.h"
#include <crypto.h>

// Special target platform header, set by configure script
#include TARGET_PLATFORM_HEADER

#define ATTEST_DATA_MAXLEN  1024
/* TODO: does not support multithreaded enclave yet */
#define MAX_ENCL_THREADS 1

#define MAX_SLAVE_ENCLAVES 9

#define MAX_MS_GROUP 10

typedef struct {
  uint64_t state;          // 状态
  unsigned int s_id;          // 从属 enclave 的 eID
  uintptr_t data_ptr;     // 数据指针
  uint64_t size;          // 数据大小
  uint64_t numbers;       // 数量
} s_enclave;

typedef struct {
  uint64_t slave_numbers; // 从属数量
  s_enclave* slave_enclave[MAX_SLAVE_ENCLAVES]; // 从属 enclave 数组
} m_enclave;

typedef struct {
  uint64_t isCreated;     // 是否已创建
  char identity[64+4];    // 身份信息
  m_enclave* m;           // 关联的 m_enclave
} ms_group;

typedef enum {
  INVALID = -1,
  DESTROYING = 0,
  ALLOCATED,
  FRESH,
  STOPPED,
  RUNNING,
} enclave_state;

/* For now, eid's are a simple unsigned int */
typedef unsigned int enclave_id;

/* Metadata around memory regions associate with this enclave
 * EPM is the 'home' for the enclave, contains runtime code/etc
 * UTM is the untrusted shared pages
 * OTHER is managed by some other component (e.g. platform_)
 * INVALID is an unused index
 */
enum enclave_region_type{
  REGION_INVALID,
  REGION_EPM,
  REGION_UTM,
  REGION_OTHER,
};

struct enclave_region
{
  region_id pmp_rid;
  enum enclave_region_type type;
};

/* enclave metadata */
struct enclave
{
  //spinlock_t lock; //local enclave lock. we don't need this until we have multithreaded enclave
  enclave_id eid; //enclave id
  unsigned long encl_satp; // enclave's page table base
  enclave_state state; // global state of the enclave

  /* Physical memory regions associate with this enclave */
  struct enclave_region regions[ENCLAVE_REGIONS_MAX];

  /* measurement */
  byte hash[MDSIZE];
  byte sign[SIGNATURE_SIZE];

  /* parameters */
  struct runtime_params_t params;

  /* enclave execution context */
  unsigned int n_thread;
  struct thread_state threads[MAX_ENCL_THREADS];

  struct platform_enclave_data ped;

  m_enclave m;
  s_enclave s;
};

/* attestation reports */
struct enclave_report
{
  byte hash[MDSIZE];
  uint64_t data_len;
  byte data[ATTEST_DATA_MAXLEN];
  byte signature[SIGNATURE_SIZE];
};
struct sm_report
{
  byte hash[MDSIZE];
  byte public_key[PUBLIC_KEY_SIZE];
  byte signature[SIGNATURE_SIZE];
};
struct report
{
  struct enclave_report enclave;
  struct sm_report sm;
  byte dev_public_key[PUBLIC_KEY_SIZE];
};

/* sealing key structure */
#define SEALING_KEY_SIZE 128
struct sealing_key
{
  uint8_t key[SEALING_KEY_SIZE];
  uint8_t signature[SIGNATURE_SIZE];
};

/*** SBI functions & external functions ***/
// callables from the host
unsigned long create_enclave(unsigned long *eid, struct keystone_sbi_create_t create_args);
unsigned long destroy_enclave(enclave_id eid);
unsigned long run_enclave(struct sbi_trap_regs *regs, enclave_id eid);
unsigned long resume_enclave(struct sbi_trap_regs *regs, enclave_id eid);
// callables from the enclave
unsigned long exit_enclave(struct sbi_trap_regs *regs, enclave_id eid);
unsigned long stop_enclave(struct sbi_trap_regs *regs, uint64_t request, enclave_id eid);
unsigned long attest_enclave(uintptr_t report, uintptr_t data, uintptr_t size, enclave_id eid);

// Function to create a group of master enclaves
unsigned long m_enclave_create_group(uintptr_t identity, uintptr_t size, enclave_id eid);

// Function to join a slave enclave to a master enclave group
unsigned long s_enclave_join_group(uintptr_t identity, uintptr_t size, enclave_id eid);

// // Function to find a master enclave group by its identity
// unsigned long s_enclave_find_group(uintptr_t identity, uintptr_t size);

// Function to get data from a slave enclave to the main enclave
unsigned long main_enclave_get_slave_enclave_data(uintptr_t dest, uintptr_t size, uintptr_t numbers, enclave_id eid);

// Function to set the data pointer for a slave enclave
unsigned long slave_enclave_set_dataptr(uintptr_t src, uintptr_t size, uintptr_t numbers, enclave_id eid);

// Function to get data from a slave enclave to the main enclave
unsigned long main_enclave_get_slave_enclave_data_yx(uintptr_t temp_ptr, uintptr_t dest_ptr, enclave_id eid);

// Function to set the data pointer for a slave enclave
unsigned long slave_enclave_set_dataptr_yx(uintptr_t temp_ptr, uintptr_t data_ptr, enclave_id eid);

unsigned long slave_enclave_set_numberblock_set_pmp(enclave_id eid);

unsigned long main_enclave_get_numberblock_set_pmp(enclave_id eid);

unsigned long other_enclave_access_stm_test_set_pmp(enclave_id eid);

// attestation
unsigned long validate_and_hash_enclave(struct enclave* enclave);
// TODO: These functions are supposed to be internal functions.
void enclave_init_metadata(void);
unsigned long copy_enclave_create_args(uintptr_t src, struct keystone_sbi_create_t* dest);
int get_enclave_region_index(enclave_id eid, enum enclave_region_type type);
uintptr_t get_enclave_region_base(enclave_id eid, int memid);
uintptr_t get_enclave_region_size(enclave_id eid, int memid);
unsigned long get_sealing_key(uintptr_t seal_key, uintptr_t key_ident, size_t key_ident_size, enclave_id eid);
// interrupt handlers
void sbi_trap_handler_keystone_enclave(struct sbi_trap_regs *regs);
#endif
