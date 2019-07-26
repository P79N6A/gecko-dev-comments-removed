










const int   g_count = 42;
const char *g_names[] = {
   "list",              
   "copy",              
   "exchange",          
   "zero",              
   "set",               
   "absolute-value",    
   "negate",            
   "add-digit",         
   "add",               
   "subtract-digit",    
   "subtract",          
   "multiply-digit",    
   "multiply",          
   "square",            
   "divide-digit",      
   "divide-2",          
   "divide-2d",         
   "divide",            
   "expt-digit",        
   "expt",              
   "expt-2",            
   "square-root",       
   "modulo-digit",      
   "modulo",            
   "mod-add",           
   "mod-subtract",      
   "mod-multiply",      
   "mod-square",        
   "mod-expt",          
   "mod-expt-digit",    
   "mod-inverse",       
   "compare-digit",     
   "compare-zero",      
   "compare",           
   "compare-magnitude", 
   "parity",            
   "gcd",               
   "lcm",               
   "conversion",        
   "binary",            
   "pprime",            
   "fermat"             
};


int  test_list(void);
int  test_copy(void);
int  test_exch(void);
int  test_zero(void);
int  test_set(void);
int  test_abs(void);
int  test_neg(void);
int  test_add_d(void);
int  test_add(void);
int  test_sub_d(void);
int  test_sub(void);
int  test_mul_d(void);
int  test_mul(void);
int  test_sqr(void);
int  test_div_d(void);
int  test_div_2(void);
int  test_div_2d(void);
int  test_div(void);
int  test_expt_d(void);
int  test_expt(void);
int  test_2expt(void);
int  test_sqrt(void);
int  test_mod_d(void);
int  test_mod(void);
int  test_addmod(void);
int  test_submod(void);
int  test_mulmod(void);
int  test_sqrmod(void);
int  test_exptmod(void);
int  test_exptmod_d(void);
int  test_invmod(void);
int  test_cmp_d(void);
int  test_cmp_z(void);
int  test_cmp(void);
int  test_cmp_mag(void);
int  test_parity(void);
int  test_gcd(void);
int  test_lcm(void);
int  test_convert(void);
int  test_raw(void);
int  test_pprime(void);
int  test_fermat(void);


int (*g_tests[])(void)  = {
   test_list,     test_copy,     test_exch,     test_zero,     
   test_set,      test_abs,      test_neg,      test_add_d,    
   test_add,      test_sub_d,    test_sub,      test_mul_d,    
   test_mul,      test_sqr,      test_div_d,    test_div_2,    
   test_div_2d,   test_div,      test_expt_d,   test_expt,     
   test_2expt,    test_sqrt,     test_mod_d,    test_mod,      
   test_addmod,   test_submod,   test_mulmod,   test_sqrmod,   
   test_exptmod,  test_exptmod_d, test_invmod,   test_cmp_d,    
   test_cmp_z,    test_cmp,      test_cmp_mag,  test_parity,   
   test_gcd,      test_lcm,      test_convert,  test_raw,      
   test_pprime,   test_fermat
};


const char *g_descs[] = {
   "print out a list of the available test suites",
   "test assignment of mp-int structures",
   "test exchange of mp-int structures",
   "test zeroing of an mp-int",
   "test setting an mp-int to a small constant",
   "test the absolute value function",
   "test the arithmetic negation function",
   "test digit addition",
   "test full addition",
   "test digit subtraction",
   "test full subtraction",
   "test digit multiplication",
   "test full multiplication",
   "test full squaring function",
   "test digit division",
   "test division by two",
   "test division & remainder by 2^d",
   "test full division",
   "test digit exponentiation",
   "test full exponentiation",
   "test power-of-two exponentiation",
   "test integer square root function",
   "test digit modular reduction",
   "test full modular reduction",
   "test modular addition",
   "test modular subtraction",
   "test modular multiplication",
   "test modular squaring function",
   "test full modular exponentiation",
   "test digit modular exponentiation",
   "test modular inverse function",
   "test digit comparison function",
   "test zero comparison function",
   "test general signed comparison",
   "test general magnitude comparison",
   "test parity comparison functions",
   "test greatest common divisor functions",
   "test least common multiple function",
   "test general radix conversion facilities",
   "test raw output format",
   "test probabilistic primality tester",
   "test Fermat pseudoprimality tester"
};

