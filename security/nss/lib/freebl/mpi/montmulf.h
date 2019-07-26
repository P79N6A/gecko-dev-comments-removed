









void conv_i32_to_d32(double *d32, unsigned int *i32, int len);









void conv_i32_to_d16(double *d16, unsigned int *i32, int len);












void conv_i32_to_d32_and_d16(double *d32, double *d16, 
			     unsigned int *i32, int len);






void mont_mulf_noconv(unsigned int *result,
		     double *dm1, double *dm2, double *dt,
		     double *dn, unsigned int *nint,
		     int nlen, double dn0);
























  
