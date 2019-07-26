
























#ifndef _TINYTEST_MACROS_H
#define _TINYTEST_MACROS_H


#define TT_STMT_BEGIN do {
#define TT_STMT_END } while (0)



#ifndef TT_EXIT_TEST_FUNCTION
#define TT_EXIT_TEST_FUNCTION TT_STMT_BEGIN goto end; TT_STMT_END
#endif


#ifndef TT_DECLARE
#define TT_DECLARE(prefix, args)				\
	TT_STMT_BEGIN						\
	printf("\n  %s %s:%d: ",prefix,__FILE__,__LINE__);	\
	printf args ;						\
	TT_STMT_END
#endif


#define TT_GRIPE(args) TT_DECLARE("FAIL", args)


#define TT_BLATHER(args)						\
	TT_STMT_BEGIN							\
	if (_tinytest_get_verbosity()>1) TT_DECLARE("  OK", args);	\
	TT_STMT_END

#define TT_DIE(args)						\
	TT_STMT_BEGIN						\
	_tinytest_set_test_failed();				\
	TT_GRIPE(args);						\
	TT_EXIT_TEST_FUNCTION;					\
	TT_STMT_END

#define TT_FAIL(args)				\
	TT_STMT_BEGIN						\
	_tinytest_set_test_failed();				\
	TT_GRIPE(args);						\
	TT_STMT_END


#define tt_abort_printf(msg) TT_DIE(msg)
#define tt_abort_perror(op) TT_DIE(("%s: %s [%d]",(op),strerror(errno), errno))
#define tt_abort_msg(msg) TT_DIE(("%s", msg))
#define tt_abort() TT_DIE(("%s", "(Failed.)"))


#define tt_fail_printf(msg) TT_FAIL(msg)
#define tt_fail_perror(op) TT_FAIL(("%s: %s [%d]",(op),strerror(errno), errno))
#define tt_fail_msg(msg) TT_FAIL(("%s", msg))
#define tt_fail() TT_FAIL(("%s", "(Failed.)"))


#define tt_skip()						\
	TT_STMT_BEGIN						\
	_tinytest_set_test_skipped();				\
	TT_EXIT_TEST_FUNCTION;					\
	TT_STMT_END

#define _tt_want(b, msg, fail)				\
	TT_STMT_BEGIN					\
	if (!(b)) {					\
		_tinytest_set_test_failed();		\
		TT_GRIPE(("%s",msg));			\
		fail;					\
	} else {					\
		TT_BLATHER(("%s",msg));			\
	}						\
	TT_STMT_END


#define tt_want_msg(b, msg)			\
	_tt_want(b, msg, );


#define tt_assert_msg(b, msg)			\
	_tt_want(b, msg, TT_EXIT_TEST_FUNCTION);


#define tt_want(b)   tt_want_msg( (b), "want("#b")")

#define tt_assert(b) tt_assert_msg((b), "assert("#b")")

#define tt_assert_test_fmt_type(a,b,str_test,type,test,printf_type,printf_fmt, \
    setup_block,cleanup_block,die_on_fail)				\
	TT_STMT_BEGIN							\
	type _val1 = (type)(a);						\
	type _val2 = (type)(b);						\
	int _tt_status = (test);					\
	if (!_tt_status || _tinytest_get_verbosity()>1)	{		\
		printf_type _print;					\
		printf_type _print1;					\
		printf_type _print2;					\
		type _value = _val1;					\
		setup_block;						\
		_print1 = _print;					\
		_value = _val2;						\
		setup_block;						\
		_print2 = _print;					\
		TT_DECLARE(_tt_status?"	 OK":"FAIL",			\
			   ("assert(%s): "printf_fmt" vs "printf_fmt,	\
			    str_test, _print1, _print2));		\
		_print = _print1;					\
		cleanup_block;						\
		_print = _print2;					\
		cleanup_block;						\
		if (!_tt_status) {					\
			_tinytest_set_test_failed();			\
			die_on_fail ;					\
		}							\
	}								\
	TT_STMT_END

#define tt_assert_test_type(a,b,str_test,type,test,fmt,die_on_fail)	\
	tt_assert_test_fmt_type(a,b,str_test,type,test,type,fmt,	\
	    {_print=_value;},{},die_on_fail)



#define tt_assert_op_type(a,op,b,type,fmt)				\
	tt_assert_test_type(a,b,#a" "#op" "#b,type,(_val1 op _val2),fmt, \
	    TT_EXIT_TEST_FUNCTION)

#define tt_int_op(a,op,b)			\
	tt_assert_test_type(a,b,#a" "#op" "#b,long,(_val1 op _val2), \
	    "%ld",TT_EXIT_TEST_FUNCTION)

#define tt_uint_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,unsigned long,		\
	    (_val1 op _val2),"%lu",TT_EXIT_TEST_FUNCTION)

#define tt_ptr_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,void*,			\
	    (_val1 op _val2),"%p",TT_EXIT_TEST_FUNCTION)

#define tt_str_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,const char *,		\
	    (strcmp(_val1,_val2) op 0),"<%s>",TT_EXIT_TEST_FUNCTION)

#define tt_want_int_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,long,(_val1 op _val2),"%ld",(void)0)

#define tt_want_uint_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,unsigned long,		\
	    (_val1 op _val2),"%lu",(void)0)

#define tt_want_ptr_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,void*,			\
	    (_val1 op _val2),"%p",(void)0)

#define tt_want_str_op(a,op,b)						\
	tt_assert_test_type(a,b,#a" "#op" "#b,const char *,		\
	    (strcmp(_val1,_val2) op 0),"<%s>",(void)0)

#endif
