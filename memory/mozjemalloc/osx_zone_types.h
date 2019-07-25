





















































typedef struct _leopard_malloc_zone {
 	void *m1;
	void *m2;
	void (*m3)();
	void (*m4)();
	void (*m5)();
	void (*m6)();
	void (*m7)();
	void (*m8)();
	void (*m9)();
	void *m10;
	void (*m11)();
	void (*m12)();
	void *m13;
	unsigned m14;
} leopard_malloc_zone;




typedef struct _snow_leopard_malloc_zone {
	void *m1;
	void *m2;
	void (*m3)();
	void (*m4)();
	void (*m5)();
	void (*m6)();
	void (*m7)();
	void (*m8)();
	void (*m9)();
	void *m10;
	void (*m11)();
	void (*m12)();
	void *m13;
	unsigned m14;
	void (*m15)(); 
	void (*m16)(); 
} snow_leopard_malloc_zone;

typedef struct _snow_leopard_malloc_introspection {
    void (*m1)();
    void (*m2)();
    void (*m3)();
    void (*m4)();
    void (*m5)();
    void (*m6)();
    void (*m7)();
    void (*m8)();
    void (*m9)(); 
} snow_leopard_malloc_introspection;




typedef struct _lion_malloc_zone {
	void *m1;
	void *m2;
	void (*m3)();
	void (*m4)();
	void (*m5)();
	void (*m6)();
	void (*m7)();
	void (*m8)();
	void (*m9)();
	void *m10;
	void (*m11)();
	void (*m12)();
	void *m13;
	unsigned m14;
	void (*m15)();
	void (*m16)();
	void (*m17)(); 
} lion_malloc_zone;

typedef struct _lion_malloc_introspection {
    void (*m1)();
    void (*m2)();
    void (*m3)();
    void (*m4)();
    void (*m5)();
    void (*m6)();
    void (*m7)();
    void (*m8)();
    void (*m9)();
    void (*m10)(); 
    void (*m11)(); 
    void (*m12)(); 
#ifdef __BLOCKS__
    void (*m13)(); 
#else
    void *m13; 
#endif
} lion_malloc_introspection;
