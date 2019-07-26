













































#include "cipher.h"
#include "rand_source.h"        
#include "alloc.h"              

debug_module_t mod_cipher = {
  0,                 
  "cipher"           
};

err_status_t
cipher_output(cipher_t *c, uint8_t *buffer, int num_octets_to_output) {
  
  
  octet_string_set_to_zero(buffer, num_octets_to_output);
  
  
  return cipher_encrypt(c, buffer, (unsigned int *) &num_octets_to_output);
}



int
cipher_get_key_length(const cipher_t *c) {
  return c->key_len;
}







#define SELF_TEST_BUF_OCTETS 128
#define NUM_RAND_TESTS       128
#define MAX_KEY_LEN          64

err_status_t
cipher_type_test(const cipher_type_t *ct, const cipher_test_case_t *test_data) {
  const cipher_test_case_t *test_case = test_data;
  cipher_t *c;
  err_status_t status;
  uint8_t buffer[SELF_TEST_BUF_OCTETS];
  uint8_t buffer2[SELF_TEST_BUF_OCTETS];
  unsigned int len;
  int i, j, case_num = 0;

  debug_print(mod_cipher, "running self-test for cipher %s", 
	      ct->description);
  
  



  if (test_case == NULL)
    return err_status_cant_check;

  


  
  while (test_case != NULL) {

    
    status = cipher_type_alloc(ct, &c, test_case->key_length_octets);
    if (status)
      return status;
    
    


    debug_print(mod_cipher, "testing encryption", NULL);    
    
    
    status = cipher_init(c, test_case->key, direction_encrypt);
    if (status) {
      cipher_dealloc(c);
      return status;
    }
    
    
    if (test_case->ciphertext_length_octets > SELF_TEST_BUF_OCTETS) {
      cipher_dealloc(c);    
      return err_status_bad_param;
    }
    for (i=0; i < test_case->plaintext_length_octets; i++)
      buffer[i] = test_case->plaintext[i];

    debug_print(mod_cipher, "plaintext:    %s",
	     octet_string_hex_string(buffer,
				     test_case->plaintext_length_octets));

    
    status = cipher_set_iv(c, test_case->idx);
    if (status) {
      cipher_dealloc(c);
      return status;
    } 
    
    
    len = test_case->plaintext_length_octets;
    status = cipher_encrypt(c, buffer, &len);
    if (status) {
      cipher_dealloc(c);
      return status;
    }
    
    debug_print(mod_cipher, "ciphertext:   %s",
	     octet_string_hex_string(buffer,
				     test_case->ciphertext_length_octets));

    
    if (len != test_case->ciphertext_length_octets)
      return err_status_algo_fail;
    status = err_status_ok;
    for (i=0; i < test_case->ciphertext_length_octets; i++)
      if (buffer[i] != test_case->ciphertext[i]) {
	status = err_status_algo_fail;
	debug_print(mod_cipher, "test case %d failed", case_num);
	debug_print(mod_cipher, "(failure at byte %d)", i);
	break;
      }
    if (status) {

      debug_print(mod_cipher, "c computed: %s",
	     octet_string_hex_string(buffer,
		  2*test_case->plaintext_length_octets));
      debug_print(mod_cipher, "c expected: %s",
		  octet_string_hex_string(test_case->ciphertext,
			  2*test_case->plaintext_length_octets));

      cipher_dealloc(c);
      return err_status_algo_fail;
    }

    


    debug_print(mod_cipher, "testing decryption", NULL);    

    
    status = cipher_init(c, test_case->key, direction_decrypt);
    if (status) {
      cipher_dealloc(c);
      return status;
    }

    
    if (test_case->ciphertext_length_octets > SELF_TEST_BUF_OCTETS) {
      cipher_dealloc(c);    
      return err_status_bad_param;
    }
    for (i=0; i < test_case->ciphertext_length_octets; i++)
      buffer[i] = test_case->ciphertext[i];

    debug_print(mod_cipher, "ciphertext:    %s",
		octet_string_hex_string(buffer,
					test_case->plaintext_length_octets));

    
    status = cipher_set_iv(c, test_case->idx);
    if (status) {
      cipher_dealloc(c);
      return status;
    } 
    
    
    len = test_case->ciphertext_length_octets;
    status = cipher_decrypt(c, buffer, &len);
    if (status) {
      cipher_dealloc(c);
      return status;
    }
    
    debug_print(mod_cipher, "plaintext:   %s",
	     octet_string_hex_string(buffer,
				     test_case->plaintext_length_octets));

    
    if (len != test_case->plaintext_length_octets)
      return err_status_algo_fail;
    status = err_status_ok;
    for (i=0; i < test_case->plaintext_length_octets; i++)
      if (buffer[i] != test_case->plaintext[i]) {
	status = err_status_algo_fail;
	debug_print(mod_cipher, "test case %d failed", case_num);
	debug_print(mod_cipher, "(failure at byte %d)", i);
      }
    if (status) {

      debug_print(mod_cipher, "p computed: %s",
	     octet_string_hex_string(buffer,
		  2*test_case->plaintext_length_octets));
      debug_print(mod_cipher, "p expected: %s",
		  octet_string_hex_string(test_case->plaintext,
			  2*test_case->plaintext_length_octets));

      cipher_dealloc(c);
      return err_status_algo_fail;
    }

    
    status = cipher_dealloc(c);
    if (status)
      return status;
    
    


   
    test_case = test_case->next_test_case;
    ++case_num;
  }
  
  

  
  test_case = test_data;
  status = cipher_type_alloc(ct, &c, test_case->key_length_octets);
  if (status)
      return status;
  
  rand_source_init();
  
  for (j=0; j < NUM_RAND_TESTS; j++) {
    unsigned length;
    int plaintext_len;
    uint8_t key[MAX_KEY_LEN];
    uint8_t  iv[MAX_KEY_LEN];

    
    length = rand() % (SELF_TEST_BUF_OCTETS - 64);
    debug_print(mod_cipher, "random plaintext length %d\n", length);
    status = rand_source_get_octet_string(buffer, length);
    if (status) return status;

    debug_print(mod_cipher, "plaintext:    %s",
		octet_string_hex_string(buffer, length));

    
    for (i=0; (unsigned int)i < length; i++)
      buffer2[i] = buffer[i];
    
    
    if (test_case->key_length_octets > MAX_KEY_LEN)
      return err_status_cant_check;
    status = rand_source_get_octet_string(key, test_case->key_length_octets);
    if (status) return status;

   
    status = rand_source_get_octet_string(iv, MAX_KEY_LEN);
    if (status) return status;
        
    
    status = cipher_init(c, key, direction_encrypt);
    if (status) {
      cipher_dealloc(c);
      return status;
    }

    
    status = cipher_set_iv(c, test_case->idx);
    if (status) {
      cipher_dealloc(c);
      return status;
    } 

    
    plaintext_len = length;
    status = cipher_encrypt(c, buffer, &length);
    if (status) {
      cipher_dealloc(c);
      return status;
    }
    debug_print(mod_cipher, "ciphertext:   %s",
		octet_string_hex_string(buffer, length));

    



    status = cipher_init(c, key, direction_decrypt);
    if (status) {
      cipher_dealloc(c);
      return status;
    }
    status = cipher_set_iv(c, test_case->idx);
    if (status) {
      cipher_dealloc(c);
      return status;
    } 
    status = cipher_decrypt(c, buffer, &length);
    if (status) {
      cipher_dealloc(c);
      return status;
    }    

    debug_print(mod_cipher, "plaintext[2]: %s",
		octet_string_hex_string(buffer, length));    

    
    if (length != plaintext_len)
      return err_status_algo_fail;
    status = err_status_ok;
    for (i=0; i < plaintext_len; i++)
      if (buffer[i] != buffer2[i]) {
	status = err_status_algo_fail;
	debug_print(mod_cipher, "random test case %d failed", case_num);
	debug_print(mod_cipher, "(failure at byte %d)", i);
      }
    if (status) {
      cipher_dealloc(c);
      return err_status_algo_fail;
    }
        
  }

  status = cipher_dealloc(c);
  if (status)
    return status;

  return err_status_ok;
}







err_status_t
cipher_type_self_test(const cipher_type_t *ct) {
  return cipher_type_test(ct, ct->test_data);
}












uint64_t
cipher_bits_per_second(cipher_t *c, int octets_in_buffer, int num_trials) {
  int i;
  v128_t nonce;
  clock_t timer;
  unsigned char *enc_buf;
  unsigned int len = octets_in_buffer;

  enc_buf = (unsigned char*) crypto_alloc(octets_in_buffer);
  if (enc_buf == NULL)
    return 0;  
  
  
  v128_set_to_zero(&nonce);
  timer = clock();
  for(i=0; i < num_trials; i++, nonce.v32[3] = i) {
    cipher_set_iv(c, &nonce);
    cipher_encrypt(c, enc_buf, &len);
  }
  timer = clock() - timer;

  crypto_free(enc_buf);

  if (timer == 0) {
    
    return 0;
  }
  
  return (uint64_t)CLOCKS_PER_SEC * num_trials * 8 * octets_in_buffer / timer;
}
