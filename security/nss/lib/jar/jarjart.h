














































#ifndef _JARJART_H_
#define _JARJART_H_


extern char *JAR_JAR_list_certs (void);


extern int JAR_JAR_validate_archive (char *filename);


extern void *JAR_JAR_new_hash (int alg);


extern void *JAR_JAR_hash (int alg, void *cookie, int length, void *data);


extern void *JAR_JAR_end_hash (int alg, void *cookie);




extern int JAR_JAR_sign_archive 
   (char *nickname, char *password, char *sf, char *outsig);


extern char *JAR_JAR_get_error (int status);

#endif
