












































#include "auth.h"



debug_module_t mod_auth = {
  0,                  
  "auth func"         
};


int
auth_get_key_length(const auth_t *a) {
  return a->key_len;
}

int
auth_get_tag_length(const auth_t *a) {
  return a->out_len;
}

int
auth_get_prefix_length(const auth_t *a) {
  return a->prefix_len;
}

int
auth_type_get_ref_count(const auth_type_t *at) {
  return at->ref_count;
}








#define SELF_TEST_TAG_BUF_OCTETS 32

err_status_t
auth_type_test(const auth_type_t *at, const auth_test_case_t *test_data) {
  const auth_test_case_t *test_case = test_data;
  auth_t *a;
  err_status_t status;
  uint8_t tag[SELF_TEST_TAG_BUF_OCTETS];
  int i, case_num = 0;

  debug_print(mod_auth, "running self-test for auth function %s", 
	      at->description);
  
  



  if (test_case == NULL)
    return err_status_cant_check;

    
  while (test_case != NULL) {

    
    if (test_case->tag_length_octets > SELF_TEST_TAG_BUF_OCTETS)
      return err_status_bad_param;
    
    
    status = auth_type_alloc(at, &a, test_case->key_length_octets,
			     test_case->tag_length_octets);
    if (status)
      return status;
    
    
    status = auth_init(a, test_case->key);
    if (status) {
      auth_dealloc(a);
      return status;
    }

    
    octet_string_set_to_zero(tag, test_case->tag_length_octets);
    status = auth_compute(a, test_case->data,
			  test_case->data_length_octets, tag);
    if (status) {
      auth_dealloc(a);
      return status;
    }
    
    debug_print(mod_auth, "key: %s",
		octet_string_hex_string(test_case->key,
					test_case->key_length_octets));
    debug_print(mod_auth, "data: %s",
		octet_string_hex_string(test_case->data,
				   test_case->data_length_octets));
    debug_print(mod_auth, "tag computed: %s",
	   octet_string_hex_string(tag, test_case->tag_length_octets));
    debug_print(mod_auth, "tag expected: %s",
	   octet_string_hex_string(test_case->tag,
				   test_case->tag_length_octets));

    
    status = err_status_ok;
    for (i=0; i < test_case->tag_length_octets; i++)
      if (tag[i] != test_case->tag[i]) {
	status = err_status_algo_fail;
	debug_print(mod_auth, "test case %d failed", case_num);
	debug_print(mod_auth, "  (mismatch at octet %d)", i);
      }
    if (status) {
      auth_dealloc(a);
      return err_status_algo_fail;
    }
    
    
    status = auth_dealloc(a);
    if (status)
      return status;
    
    


   
    test_case = test_case->next_test_case;
    ++case_num;
  }
  
  return err_status_ok;
}







err_status_t
auth_type_self_test(const auth_type_t *at) {
  return auth_type_test(at, at->test_data);
}

