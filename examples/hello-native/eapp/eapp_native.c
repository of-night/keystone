//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "eapp_utils.h"
#include "string.h"
#include "edge_call.h"
#include <syscall.h>

#define OCALL_PRINT_STRING 1

int secret_data = 0xDEADBEEF;

uintptr_t test = &secret_data;
unsigned long ocall_print_string(uintptr_t* string);

int main(){

  // ocall_print_string("Hello World");
  ocall_print_string(&test);

    //   // 使用 edge_call 获取共享内存的指针
    // edge_data_offset shared_data;
    // uintptr_t* shared_buffer = NULL;
    // if (edge_call_get_ptr_from_offset(shared_data, 0, shared_buffer) == 0) {
    //     *(*shared_buffer) = &secret_data;
    //     ocall_print_string("Enclave: Secret data copied to shared memory: ????\n");
    // } else {
    //     ocall_print_string("Enclave: Error - Unable to access shared memory\n");
    // }

  EAPP_RETURN(0);
}

unsigned long ocall_print_string(uintptr_t* string){
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, 4, &retval ,sizeof(unsigned long));
  return retval;
}

