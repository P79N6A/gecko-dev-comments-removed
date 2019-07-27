




import json

from mozharness.base import log
from mozharness.base.log import OutputParser, WARNING, INFO, ERROR
from mozharness.mozilla.buildbot import TBPL_WARNING, TBPL_FAILURE
from mozharness.mozilla.buildbot import TBPL_SUCCESS, TBPL_WORST_LEVEL_TUPLE
from mozharness.mozilla.testing.unittest import tbox_print_summary


class StructuredOutputParser(OutputParser):
    
    
    def __init__(self, **kwargs):
        """Object that tracks the overall status of the test run"""
        
        
        
        
        if 'strict' in kwargs:
            self.strict = kwargs.pop('strict')
        else:
            self.strict = True

        self.suite_category = kwargs.pop('suite_category', None)

        super(StructuredOutputParser, self).__init__(**kwargs)

        structured = self._get_mozlog_module()
        self.formatter = structured.formatters.TbplFormatter()
        self.handler = structured.handlers.StatusHandler()
        self.log_actions = structured.structuredlog.log_actions()

        self.worst_log_level = INFO
        self.tbpl_status = TBPL_SUCCESS

    def _get_mozlog_module(self):
        try:
            from mozlog import structured
        except ImportError:
            self.fatal("A script class using structured logging must inherit "
                       "from the MozbaseMixin to ensure that mozlog is available.")
        return structured

    def _handle_unstructured_output(self, line):
        if self.strict:
            self.critical(("Test harness output was not a valid structured log message: "
                          "\n%s") % line)
            self.update_levels(TBPL_FAILURE, log.CRITICAL)
            return
        super(StructuredOutputParser, self).parse_single_line(line)


    def parse_single_line(self, line):
        """Parses a line of log output from the child process and passes
        it to mozlog to update the overall status of the run.
        Re-emits the logged line in human-readable format.
        """
        level = INFO
        tbpl_level = TBPL_SUCCESS

        data = None
        try:
            candidate_data = json.loads(line)
            if (isinstance(candidate_data, dict) and
                'action' in candidate_data and candidate_data['action'] in self.log_actions):
                data = candidate_data
        except ValueError:
            pass

        if data is None:
            self._handle_unstructured_output(line)
            return

        self.handler(data)

        action = data["action"]
        if action == "log":
            level = getattr(log, data["level"].upper())

        self.log(self.formatter(data), level=level)
        self.update_levels(tbpl_level, level)

    def evaluate_parser(self, return_code, success_codes=None):
        success_codes = success_codes or [0]
        summary = self.handler.summarize()

        fail_pair = TBPL_WARNING, WARNING
        error_pair = TBPL_FAILURE, ERROR

        
        failure_conditions = [
            sum(summary.unexpected_statuses.values()) > 0,
            summary.action_counts.get('crash', 0) > summary.expected_statuses.get('CRASH', 0),
        ]
        for condition in failure_conditions:
            if condition:
                self.update_levels(*fail_pair)

        
        
        required_actions = {
            'suite_end': 'No suite end message was emitted by this harness.',
            'test_end': 'No checks run.',
        }
        for action, diagnostic_message in required_actions.iteritems():
            if action not in summary.action_counts:
                self.log(diagnostic_message, ERROR)
                self.update_levels(*error_pair)

        failure_log_levels = ['ERROR', 'CRITICAL']
        for level in failure_log_levels:
            if level in summary.log_level_counts:
                self.update_levels(*error_pair)

        
        
        if self.num_errors:
            self.update_levels(*error_pair)

        
        
        if return_code not in success_codes and self.tbpl_status == TBPL_SUCCESS:
            self.update_levels(*error_pair)

        return self.tbpl_status, self.worst_log_level

    def update_levels(self, tbpl_level, log_level):
        self.worst_log_level = self.worst_level(log_level, self.worst_log_level)
        self.tbpl_status = self.worst_level(tbpl_level, self.tbpl_status,
                                            levels=TBPL_WORST_LEVEL_TUPLE)

    def print_summary(self, suite_name):
        
        
        
        
        
        summary = self.handler.summarize()
        unexpected_count = sum(summary.unexpected_statuses.values())
        expected_count = sum(summary.expected_statuses.values())
        expected_failures = summary.expected_statuses.get('FAIL', 0)

        if unexpected_count:
            fail_text = '<em class="testfail">%s</em>' % unexpected_count
        else:
            fail_text = '0'

        text_summary = "%s/%s/%s" % (expected_count, fail_text, expected_failures)
        self.info("TinderboxPrint: %s: %s\n" % (suite_name, text_summary))

    def append_tinderboxprint_line(self, suite_name):
        summary = self.handler.summarize()
        unexpected_count = sum(summary.unexpected_statuses.values())
        expected_count = sum(summary.expected_statuses.values())
        expected_failures = summary.expected_statuses.get('FAIL', 0)
        crashed = 0
        if 'crash' in summary.action_counts:
            crashed = summary.action_counts['crash']
        text_summary = tbox_print_summary(expected_count,
                                          unexpected_count,
                                          expected_failures,
                                          crashed > 0,
                                          False)
        self.info("TinderboxPrint: %s<br/>%s\n" % (suite_name, text_summary))
