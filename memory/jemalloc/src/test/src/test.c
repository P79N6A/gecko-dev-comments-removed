#include "test/jemalloc_test.h"

static unsigned		test_count = 0;
static test_status_t	test_counts[test_status_count] = {0, 0, 0};
static test_status_t	test_status = test_status_pass;
static const char *	test_name = "";

JEMALLOC_ATTR(format(printf, 1, 2))
void
test_skip(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	malloc_vcprintf(NULL, NULL, format, ap);
	va_end(ap);
	malloc_printf("\n");
	test_status = test_status_skip;
}

JEMALLOC_ATTR(format(printf, 1, 2))
void
test_fail(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	malloc_vcprintf(NULL, NULL, format, ap);
	va_end(ap);
	malloc_printf("\n");
	test_status = test_status_fail;
}

static const char *
test_status_string(test_status_t test_status)
{

	switch (test_status) {
	case test_status_pass: return "pass";
	case test_status_skip: return "skip";
	case test_status_fail: return "fail";
	default: not_reached();
	}
}

void
p_test_init(const char *name)
{

	test_count++;
	test_status = test_status_pass;
	test_name = name;
}

void
p_test_fini(void)
{

	test_counts[test_status]++;
	malloc_printf("%s: %s\n", test_name, test_status_string(test_status));
}

test_status_t
p_test(test_t *t, ...)
{
	test_status_t ret;
	va_list ap;

	






	if (nallocx(1, 0) == 0) {
		malloc_printf("Initialization error");
		return (test_status_fail);
	}

	ret = test_status_pass;
	va_start(ap, t);
	for (; t != NULL; t = va_arg(ap, test_t *)) {
		t();
		if (test_status > ret)
			ret = test_status;
	}
	va_end(ap);

	malloc_printf("--- %s: %u/%u, %s: %u/%u, %s: %u/%u ---\n",
	    test_status_string(test_status_pass),
	    test_counts[test_status_pass], test_count,
	    test_status_string(test_status_skip),
	    test_counts[test_status_skip], test_count,
	    test_status_string(test_status_fail),
	    test_counts[test_status_fail], test_count);

	return (ret);
}

void
p_test_fail(const char *prefix, const char *message)
{

	malloc_cprintf(NULL, NULL, "%s%s\n", prefix, message);
	test_status = test_status_fail;
}
