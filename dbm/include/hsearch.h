




































typedef struct entry {
	char *key;
	char *data;
} ENTRY;

typedef enum {
	FIND, ENTER
} ACTION;

int	 hcreate (unsigned int);
void	 hdestroy (void);
ENTRY	*hsearch (ENTRY, ACTION);
