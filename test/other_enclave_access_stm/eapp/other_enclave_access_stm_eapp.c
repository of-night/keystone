#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/malloc.h"
#include "app/syscall.h"
#include "edge/edge_common.h"
#include <stdio.h>

#define OCALL_PRINT_STRING      1

unsigned long ocall_print_string(char* string);
void ocall_dispath_get_wrapper(struct edge_data* retdata);

int main() {

    struct edge_data edgedata;
    long long start_offset = 0;

    ocall_print_string("hello, world! 1");

    ocall_print_string("hello, world! 2");

    // test
    if (start_offset == 1) {
        other_enclave_access_stm_test();
    }

    ocall_print_string("hello, world! 3");

    return 0;
}

unsigned long ocall_print_string(char* string){
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string)+1, &retval ,sizeof(unsigned long));
  return retval;
}
