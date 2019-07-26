











































#include "stat.h"

debug_module_t mod_stat = {
  0,                 
  (char *)"stat test"        
};






#define STAT_TEST_DATA_LEN 2500

err_status_t
stat_test_monobit(uint8_t *data) {
  uint8_t *data_end = data + STAT_TEST_DATA_LEN;
  uint16_t ones_count;

  ones_count = 0;
  while (data < data_end) {
    ones_count += octet_get_weight(*data);
    data++;
  }

  debug_print(mod_stat, "bit count: %d", ones_count);
  
  if ((ones_count < 9725) || (ones_count > 10275))
    return err_status_algo_fail;

  return err_status_ok;
}

err_status_t
stat_test_poker(uint8_t *data) {
  int i;
  uint8_t *data_end = data + STAT_TEST_DATA_LEN;
  double poker;
  uint16_t f[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
  };
  
  while (data < data_end) {
    f[*data & 0x0f]++;    
    f[(*data) >> 4]++;    
    data++;
  }

  poker = 0.0;
  for (i=0; i < 16; i++) 
    poker += (double) f[i] * f[i];

  poker *= (16.0 / 5000.0);
  poker -= 5000.0;

  debug_print(mod_stat, "poker test: %f\n", poker);
    
  if ((poker < 2.16) || (poker > 46.17))
    return err_status_algo_fail;
  
  return err_status_ok;
}






err_status_t
stat_test_runs(uint8_t *data) {
  uint8_t *data_end = data + STAT_TEST_DATA_LEN;
  uint16_t runs[6] = { 0, 0, 0, 0, 0, 0 }; 
  uint16_t gaps[6] = { 0, 0, 0, 0, 0, 0 };
  uint16_t lo_value[6] = { 2315, 1114, 527, 240, 103, 103 };
  uint16_t hi_value[6] = { 2685, 1386, 723, 384, 209, 209 };
  int state = 0;
  uint16_t mask;
  int i;
  
  



  
  while (data < data_end) {

    
    for (mask = 1; mask < 256; mask <<= 1) {
      if (*data & mask) {

 	
	if (state > 0) {

	  
	  state++;                          

	   
	  if (state > 25) {
		debug_print(mod_stat, ">25 runs: %d", state);
		return err_status_algo_fail;
	  }

	} else if (state < 0) {

	  
	  if (state < -25) {
		debug_print(mod_stat, ">25 gaps: %d", state);
	    return err_status_algo_fail;    
	  }
	  if (state < -6) {
	    state = -6;                     
	  }
	  gaps[-1-state]++;                 
          state = 1;                        
	} else {

	  
	  state = 1;            
	}
      } else {

	
	if (state > 0) {

	  
	  if (state > 25) {
		debug_print(mod_stat, ">25 runs (2): %d", state);
	    return err_status_algo_fail;    
	  }
	  if (state > 6) {
	    state = 6;                      
	  }
	  runs[state-1]++;                  
          state = -1;                       
	} else if (state < 0) {

	  
	  state--;

	   
	  if (state < -25) {
		debug_print(mod_stat, ">25 gaps (2): %d", state);
	    return err_status_algo_fail;
	  }

	} else {

	  
	  state = -1;
	}
      }
    }

    
    data++;
  }

  if (mod_stat.on) {
    debug_print(mod_stat, "runs test", NULL);
    for (i=0; i < 6; i++)
      debug_print(mod_stat, "  runs[]: %d", runs[i]);
    for (i=0; i < 6; i++)
      debug_print(mod_stat, "  gaps[]: %d", gaps[i]);
  }

  
  for (i=0; i < 6; i++) 
    if (   (runs[i] < lo_value[i] ) || (runs[i] > hi_value[i])
	|| (gaps[i] < lo_value[i] ) || (gaps[i] > hi_value[i]))
      return err_status_algo_fail;

  
  return err_status_ok;
}








#define RAND_SRC_BUF_OCTETS 50 /* this value MUST divide 2500! */ 

err_status_t
stat_test_rand_source(rand_source_func_t get_rand_bytes) {
  int i;
  double poker;
  uint8_t *data, *data_end;
  uint16_t f[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
  };
  uint8_t buffer[RAND_SRC_BUF_OCTETS];
  err_status_t status;
  int ones_count = 0;
  uint16_t runs[6] = { 0, 0, 0, 0, 0, 0 }; 
  uint16_t gaps[6] = { 0, 0, 0, 0, 0, 0 };
  uint16_t lo_value[6] = { 2315, 1114, 527, 240, 103, 103 };
  uint16_t hi_value[6] = { 2685, 1386, 723, 384, 209, 209 };
  int state = 0;
  uint16_t mask;
  
  

  
  for (i=0; i < 2500; i+=RAND_SRC_BUF_OCTETS) {
    
    
    status = get_rand_bytes(buffer, RAND_SRC_BUF_OCTETS);
    if (status) {
	  debug_print(mod_stat, "couldn't get rand bytes: %d",status);
      return status;
	}

#if 0
    debug_print(mod_stat, "%s", 
		octet_string_hex_string(buffer, RAND_SRC_BUF_OCTETS));
#endif
  
    data = buffer;
    data_end = data + RAND_SRC_BUF_OCTETS;
    while (data < data_end) {

      
      ones_count += octet_get_weight(*data);

      
      f[*data & 0x0f]++;    
      f[(*data) >> 4]++;    

      
      
      for (mask = 1; mask < 256; mask <<= 1) {
	if (*data & mask) {
	  
	  
	  if (state > 0) {
	    
	    
	    state++;                          
	    
	     
	    if (state > 25) {
		  debug_print(mod_stat, ">25 runs (3): %d", state);
	      return err_status_algo_fail;
		}
	    
	  } else if (state < 0) {
	    
	    
	    if (state < -25) {
		  debug_print(mod_stat, ">25 gaps (3): %d", state);
	      return err_status_algo_fail;    
	    }
	    if (state < -6) {
	      state = -6;                     
	    }
	    gaps[-1-state]++;                 
	    state = 1;                        
	  } else {
	    
	    
	    state = 1;            
	  }
	} else {
	  
	  
	  if (state > 0) {
	    
	    
	    if (state > 25) {
		  debug_print(mod_stat, ">25 runs (4): %d", state);
	      return err_status_algo_fail;    
	    }
	    if (state > 6) {
	      state = 6;                      
	    }
	    runs[state-1]++;                  
	    state = -1;                       
	  } else if (state < 0) {
	    
	    
	    state--;
	    
	     
	    if (state < -25) {
		  debug_print(mod_stat, ">25 gaps (4): %d", state);
	      return err_status_algo_fail;
		}
	    
	  } else {
	    
	    
	    state = -1;
	  }
	}
      }
      
      
      data++;
    }
  }

  

  

  debug_print(mod_stat, "stat: bit count: %d", ones_count);
  
  if ((ones_count < 9725) || (ones_count > 10275)) {
    debug_print(mod_stat, "stat: failed monobit test %d", ones_count);
    return err_status_algo_fail;
  }
  
  
  poker = 0.0;
  for (i=0; i < 16; i++) 
    poker += (double) f[i] * f[i];

  poker *= (16.0 / 5000.0);
  poker -= 5000.0;

  debug_print(mod_stat, "stat: poker test: %f", poker);
    
  if ((poker < 2.16) || (poker > 46.17)) {
    debug_print(mod_stat, "stat: failed poker test", NULL);
    return err_status_algo_fail;
  }

  
  for (i=0; i < 6; i++) 
    if ((runs[i] < lo_value[i] ) || (runs[i] > hi_value[i])
	 || (gaps[i] < lo_value[i] ) || (gaps[i] > hi_value[i])) {
      debug_print(mod_stat, "stat: failed run/gap test", NULL);
      return err_status_algo_fail; 
    }

  debug_print(mod_stat, "passed random stat test", NULL);
  return err_status_ok;
}

err_status_t
stat_test_rand_source_with_repetition(rand_source_func_t source, unsigned num_trials) {
  unsigned int i;
  err_status_t err = err_status_algo_fail;

  for (i=0; i < num_trials; i++) {
    err = stat_test_rand_source(source);
    if (err == err_status_ok) {
      return err_status_ok;  
    }
    debug_print(mod_stat, "failed stat test (try number %d)\n", i);
  }
  
  return err;
}
