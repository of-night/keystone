#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/malloc.h"
#include "app/syscall.h"
#include "edge/edge_common.h"
#include <stdio.h>

#define OCALL_PRINT_STRING      1
#define OCALL_STARTOFFSET_GET   2

unsigned long ocall_print_string(char* string);
void ocall_dispath_get_wrapper(struct edge_data* retdata);

int main() {

    ocall_print_string("hello, world! 1");

    struct edge_data edgedata;
    long long start_offset = 0;

    ocall_dispath_get_wrapper(&edgedata);
    if (edgedata.size != sizeof(start_offset)) {
        ocall_print_string("get start_offset size error");
        return 0;
    }
    copy_from_shared((void*)&start_offset, edgedata.offset, edgedata.size);

    // test
    if (start_offset == 1) {
        other_enclave_access_stm_test();
    } else {
        ocall_dispath_get_wrapper(&edgedata);
        if (edgedata.size != sizeof(start_offset)) {
            ocall_print_string("get start_offset size error");
            return 0;
        }
        copy_from_shared((void*)start_offset, edgedata.offset, edgedata.size);
    }

    ocall_print_string("hello, world! 2");

    return 0;
}

unsigned long ocall_print_string(char* string){
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string)+1, &retval ,sizeof(unsigned long));
  return retval;
}

void ocall_dispath_get_wrapper(struct edge_data* retdata){
    ocall(OCALL_STARTOFFSET_GET, NULL, 0, retdata, sizeof(struct edge_data));
    return;
}