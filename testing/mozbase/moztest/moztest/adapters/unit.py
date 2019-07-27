



import unittest
import sys
import time
import traceback

try:
    from unittest import TextTestResult
except ImportError:
    
    from unittest import _TextTestResult as TextTestResult

"""Adapter used to output structuredlog messages from unittest
testsuites"""


class StructuredTestResult(TextTestResult):
    def __init__(self, *args, **kwargs):
        self.logger = kwargs.pop('logger')
        self.test_list = kwargs.pop("test_list", [])
        self.result_callbacks = kwargs.pop('result_callbacks', [])
        self.passed = 0
        self.testsRun = 0
        TextTestResult.__init__(self, *args, **kwargs)

    def call_callbacks(self, test, status):
        debug_info = {}
        for callback in self.result_callbacks:
            info = callback(test, status)
            if info is not None:
                debug_info.update(info)
        return debug_info

    def startTestRun(self):
        
        
        
        pass

    def startTest(self, test):
        self.testsRun += 1
        self.logger.test_start(test.id())

    def stopTest(self, test):
        pass

    def stopTestRun(self):
        
        
        
        pass

    def _extract_err_message(self, err):
        
        
        exc_ty, val, _ = err
        exc_msg = "".join(traceback.format_exception_only(exc_ty, val))
        if self.buffer:
            output_msg = "\n".join([sys.stdout.getvalue(), sys.stderr.getvalue()])
            return "".join([exc_msg, output_msg])
        return exc_msg.rstrip()

    def _extract_stacktrace(self, err, test):
        
        
        

        exctype, value, tb = err
        while tb and self._is_relevant_tb_level(tb):
            tb = tb.tb_next
        
        lines = ["Traceback (most recent call last):\n"]
        if exctype is test.failureException:
            length = self._count_relevant_tb_levels(tb)
            lines += traceback.format_tb(tb, length)
        else:
            lines += traceback.format_tb(tb)
        return "".join(lines)

    def addError(self, test, err):
        self.errors.append((test, self._exc_info_to_string(err, test)))
        extra = self.call_callbacks(test, "ERROR")
        self.logger.test_end(test.id(),
                             "ERROR",
                             message=self._extract_err_message(err),
                             expected="PASS",
                             stack=self._extract_stacktrace(err, test),
                             extra=extra)

    def addFailure(self, test, err):
        extra = self.call_callbacks(test, "ERROR")
        self.logger.test_end(test.id(),
                            "FAIL",
                             message=self._extract_err_message(err),
                             expected="PASS",
                             stack=self._extract_stacktrace(err, test),
                             extra=extra)

    def addSuccess(self, test):
        self.logger.test_end(test.id(), "PASS", expected="PASS")

    def addExpectedFailure(self, test, err):
        extra = self.call_callbacks(test, "ERROR")
        self.logger.test_end(test.id(),
                            "FAIL",
                             message=self._extract_err_message(err),
                             expected="FAIL",
                             stack=self._extract_stacktrace(err, test),
                             extra=extra)

    def addUnexpectedSuccess(self, test):
        extra = self.call_callbacks(test, "ERROR")
        self.logger.test_end(test.id(),
                             "PASS",
                             expected="FAIL",
                             extra=extra)

    def addSkip(self, test, reason):
        extra = self.call_callbacks(test, "ERROR")
        self.logger.test_end(test.id(),
                             "SKIP",
                             message=reason,
                             expected="PASS",
                             extra=extra)


class StructuredTestRunner(unittest.TextTestRunner):

    resultclass = StructuredTestResult

    def __init__(self, **kwargs):
        """TestRunner subclass designed for structured logging.

        :params logger: A ``StructuredLogger`` to use for logging the test run.
        :params test_list: An optional list of tests that will be passed along
            the `suite_start` message.

        """

        self.logger = kwargs.pop("logger")
        self.test_list = kwargs.pop("test_list", [])
        self.result_callbacks = kwargs.pop("result_callbacks", [])
        unittest.TextTestRunner.__init__(self, **kwargs)

    def _makeResult(self):
        return self.resultclass(self.stream,
                                self.descriptions,
                                self.verbosity,
                                logger=self.logger,
                                test_list=self.test_list)

    def run(self, test):
        """Run the given test case or test suite."""
        result = self._makeResult()
        result.failfast = self.failfast
        result.buffer = self.buffer
        startTime = time.time()
        startTestRun = getattr(result, 'startTestRun', None)
        if startTestRun is not None:
            startTestRun()
        try:
            test(result)
        finally:
            stopTestRun = getattr(result, 'stopTestRun', None)
            if stopTestRun is not None:
                stopTestRun()
        stopTime = time.time()
        if hasattr(result, 'time_taken'):
            result.time_taken = stopTime - startTime

        return result
