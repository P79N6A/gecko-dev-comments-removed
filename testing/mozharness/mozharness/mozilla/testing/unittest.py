






import os
import re

from mozharness.mozilla.testing.errors import TinderBoxPrintRe
from mozharness.base.log import OutputParser, WARNING, INFO, CRITICAL
from mozharness.mozilla.buildbot import TBPL_WARNING, TBPL_FAILURE, TBPL_RETRY
from mozharness.mozilla.buildbot import TBPL_SUCCESS, TBPL_WORST_LEVEL_TUPLE

SUITE_CATEGORIES = ['mochitest', 'reftest', 'xpcshell']


def tbox_print_summary(pass_count, fail_count, known_fail_count=None,
                       crashed=False, leaked=False):
    emphasize_fail_text = '<em class="testfail">%s</em>'

    if pass_count < 0 or fail_count < 0 or \
            (known_fail_count is not None and known_fail_count < 0):
        summary = emphasize_fail_text % 'T-FAIL'
    elif pass_count == 0 and fail_count == 0 and \
            (known_fail_count == 0 or known_fail_count is None):
        summary = emphasize_fail_text % 'T-FAIL'
    else:
        str_fail_count = str(fail_count)
        if fail_count > 0:
            str_fail_count = emphasize_fail_text % str_fail_count
        summary = "%d/%s" % (pass_count, str_fail_count)
        if known_fail_count is not None:
            summary += "/%d" % known_fail_count
    
    if crashed:
        summary += "&nbsp;%s" % emphasize_fail_text % "CRASH"
    
    if leaked is not False:
        summary += "&nbsp;%s" % emphasize_fail_text % (
            (leaked and "LEAK") or "L-FAIL")
    return summary


class TestSummaryOutputParserHelper(OutputParser):
    def __init__(self, regex=re.compile(r'(passed|failed|todo): (\d+)'), **kwargs):
        self.regex = regex
        self.failed = 0
        self.passed = 0
        self.todo = 0
        self.last_line = None
        super(TestSummaryOutputParserHelper, self).__init__(**kwargs)

    def parse_single_line(self, line):
        super(TestSummaryOutputParserHelper, self).parse_single_line(line)
        self.last_line = line
        m = self.regex.search(line)
        if m:
            try:
                setattr(self, m.group(1), int(m.group(2)))
            except ValueError:
                
                pass

    def evaluate_parser(self):
        
        emphasize_fail_text = '<em class="testfail">%s</em>'
        failed = "0"
        if self.passed == 0 and self.failed == 0:
            self.tsummary = emphasize_fail_text % "T-FAIL"
        else:
            if self.failed > 0:
                failed = emphasize_fail_text % str(self.failed)
            self.tsummary = "%d/%s/%d" % (self.passed, failed, self.todo)

    def print_summary(self, suite_name):
        self.evaluate_parser()
        self.info("TinderboxPrint: %s: %s\n" % (suite_name, self.tsummary))


class DesktopUnittestOutputParser(OutputParser):
    """
    A class that extends OutputParser such that it can parse the number of
    passed/failed/todo tests from the output.
    """

    def __init__(self, suite_category, **kwargs):
        
        
        self.worst_log_level = INFO
        super(DesktopUnittestOutputParser, self).__init__(**kwargs)
        self.summary_suite_re = TinderBoxPrintRe.get('%s_summary' % suite_category, {})
        self.harness_error_re = TinderBoxPrintRe['harness_error']['minimum_regex']
        self.full_harness_error_re = TinderBoxPrintRe['harness_error']['full_regex']
        self.harness_retry_re = TinderBoxPrintRe['harness_error']['retry_regex']
        self.fail_count = -1
        self.pass_count = -1
        
        self.known_fail_count = self.summary_suite_re.get('known_fail_group') and -1
        self.crashed, self.leaked = False, False
        self.tbpl_status = TBPL_SUCCESS

    def parse_single_line(self, line):
        if self.summary_suite_re:
            summary_m = self.summary_suite_re['regex'].match(line)  
            if summary_m:
                message = ' %s' % line
                log_level = INFO
                
                
                summary_match_list = [group for group in summary_m.groups()
                                      if group is not None]
                r = summary_match_list[0]
                if self.summary_suite_re['pass_group'] in r:
                    if len(summary_match_list) > 1:
                        self.pass_count = int(summary_match_list[-1])
                    else:
                        
                        
                        
                        self.pass_count = 1
                        self.fail_count = 0
                elif self.summary_suite_re['fail_group'] in r:
                    self.fail_count = int(summary_match_list[-1])
                    if self.fail_count > 0:
                        message += '\n One or more unittests failed.'
                        log_level = WARNING
                
                
                elif self.summary_suite_re['known_fail_group'] in r:
                    self.known_fail_count = int(summary_match_list[-1])
                self.log(message, log_level)
                return  
        harness_match = self.harness_error_re.match(line)
        if harness_match:
            self.warning(' %s' % line)
            self.worst_log_level = self.worst_level(WARNING, self.worst_log_level)
            self.tbpl_status = self.worst_level(TBPL_WARNING, self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)
            full_harness_match = self.full_harness_error_re.match(line)
            if full_harness_match:
                r = full_harness_match.group(1)
                if r == "application crashed":
                    self.crashed = True
                elif r == "missing output line for total leaks!":
                    self.leaked = None
                else:
                    self.leaked = True
            return  
        if self.harness_retry_re.search(line):
            self.critical(' %s' % line)
            self.worst_log_level = self.worst_level(CRITICAL, self.worst_log_level)
            self.tbpl_status = self.worst_level(TBPL_RETRY, self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)
            return  
        super(DesktopUnittestOutputParser, self).parse_single_line(line)

    def evaluate_parser(self, return_code, success_codes=None):
        success_codes = success_codes or [0]

        if self.num_errors:  
            self.tbpl_status = self.worst_level(TBPL_FAILURE, self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)

        
        
        
        if self.fail_count != 0:
            self.worst_log_level = self.worst_level(WARNING, self.worst_log_level)
            self.tbpl_status = self.worst_level(TBPL_WARNING, self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)

        
        if self.pass_count <= 0 and self.fail_count <= 0 and \
            (self.known_fail_count is None or self.known_fail_count <= 0):
            self.error('No tests run or test summary not found')
            self.worst_log_level = self.worst_level(WARNING,
                                                    self.worst_log_level)
            self.tbpl_status = self.worst_level(TBPL_WARNING,
                                                self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)

        if return_code not in success_codes:
            self.tbpl_status = self.worst_level(TBPL_FAILURE, self.tbpl_status,
                                                levels=TBPL_WORST_LEVEL_TUPLE)

        
        return (self.tbpl_status, self.worst_log_level)

    def append_tinderboxprint_line(self, suite_name):
        
        
        
        
        
        summary = tbox_print_summary(self.pass_count,
                                     self.fail_count,
                                     self.known_fail_count,
                                     self.crashed,
                                     self.leaked)
        self.info("TinderboxPrint: %s<br/>%s\n" % (suite_name, summary))


class EmulatorMixin(object):
    """ Currently dependent on both TooltoolMixin and TestingMixin)"""

    def install_emulator_from_tooltool(self, manifest_path, do_unzip=True):
        dirs = self.query_abs_dirs()
        if self.tooltool_fetch(manifest_path, output_dir=dirs['abs_work_dir'],
                               cache=self.config.get("tooltool_cache", None)
                               ):
            self.fatal("Unable to download emulator via tooltool!")
        if do_unzip:
            unzip = self.query_exe("unzip")
            unzip_cmd = [unzip, '-q', os.path.join(dirs['abs_work_dir'], "emulator.zip")]
            self.run_command(unzip_cmd, cwd=dirs['abs_emulator_dir'], halt_on_failure=True,
                             fatal_exit_code=3)

    def install_emulator(self):
        dirs = self.query_abs_dirs()
        self.mkdir_p(dirs['abs_emulator_dir'])
        if self.config.get('emulator_url'):
            self._download_unzip(self.config['emulator_url'], dirs['abs_emulator_dir'])
        elif self.config.get('emulator_manifest'):
            manifest_path = self.create_tooltool_manifest(self.config['emulator_manifest'])
            do_unzip = True
            if 'unpack' in self.config['emulator_manifest']:
                do_unzip = False
            self.install_emulator_from_tooltool(manifest_path, do_unzip)
        elif self.buildbot_config:
            props = self.buildbot_config.get('properties')
            url = 'https://hg.mozilla.org/%s/raw-file/%s/b2g/test/emulator.manifest' % (
                props['repo_path'], props['revision'])
            manifest_path = self.download_file(url,
                                               file_name='tooltool.tt',
                                               parent_dir=dirs['abs_work_dir'])
            if not manifest_path:
                self.fatal("Can't download emulator manifest from %s" % url)
            self.install_emulator_from_tooltool(manifest_path)
        else:
            self.fatal("Can't get emulator; set emulator_url or emulator_manifest in the config!")
        if self.config.get('tools_manifest'):
            manifest_path = self.create_tooltool_manifest(self.config['tools_manifest'])
            do_unzip = True
            if 'unpack' in self.config['tools_manifest']:
                do_unzip = False
            self.install_emulator_from_tooltool(manifest_path, do_unzip)
