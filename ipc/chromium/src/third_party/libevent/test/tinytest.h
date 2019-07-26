
























#ifndef _TINYTEST_H
#define _TINYTEST_H


#define TT_FORK  (1<<0)

#define TT_SKIP  (1<<1)

#define _TT_ENABLED  (1<<2)

#define TT_FIRST_USER_FLAG (1<<3)

typedef void (*testcase_fn)(void *);

struct testcase_t;


struct testcase_setup_t {
	
	void *(*setup_fn)(const struct testcase_t *);
	
	int (*cleanup_fn)(const struct testcase_t *, void *);
};


struct testcase_t {
	const char *name; 
	testcase_fn fn; 
	unsigned long flags; 
	const struct testcase_setup_t *setup; 
	void *setup_data; 
};
#define END_OF_TESTCASES { NULL, NULL, 0, NULL, NULL }


struct testgroup_t {
	const char *prefix; 
	struct testcase_t *cases; 
};
#define END_OF_GROUPS { NULL, NULL}


void _tinytest_set_test_failed(void);

void _tinytest_set_test_skipped(void);

int _tinytest_get_verbosity(void);


int _tinytest_set_flag(struct testgroup_t *, const char *, unsigned long);


#define tinytest_skip(groups, named) \
	_tinytest_set_flag(groups, named, TT_SKIP)


int testcase_run_one(const struct testgroup_t *,const struct testcase_t *);


int tinytest_main(int argc, const char **argv, struct testgroup_t *groups);

#endif
