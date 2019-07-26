






#ifndef ZIP_H
#define ZIP_H










typedef struct ZIPentry_s {
	struct ZipLocal local;		
	struct ZipCentral central;	
	char *filename;				
	char *comment;				

	struct ZIPentry_s *next;
} ZIPentry;




typedef struct ZIPfile_s {
	char *filename;	
	char *comment;	
	PRFileDesc *fp;	
	ZIPentry *list;	
	unsigned int time;	
	unsigned int date;  
	unsigned long central_start; 
	unsigned long central_end; 
} ZIPfile;






ZIPfile* JzipOpen(char *filename, char *comment);









int JzipAdd(char *fullname, char *filename, ZIPfile *zipfile,
	int compression_level);








int JzipClose (ZIPfile *zipfile);


#endif 
