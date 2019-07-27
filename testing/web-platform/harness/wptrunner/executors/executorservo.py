



import json
import subprocess
import threading
import urlparse

from mozprocess import ProcessHandler

from .base import testharness_result_converter
from .process import ProcessTestExecutor


class ServoTestharnessExecutor(ProcessTestExecutor):
    convert_result = testharness_result_converter

    def __init__(self, *args, **kwargs):
        ProcessTestExecutor.__init__(self, *args, **kwargs)
        self.result_data = None
        self.result_flag = None

    def run_test(self, test):
        self.result_data = None
        self.result_flag = threading.Event()

        self.command = [self.binary, "--cpu", "--hard-fail",
                        urlparse.urljoin(self.http_server_url, test.url)]

        if self.debug_args:
            self.command = list(self.debug_args) + self.command


        self.proc = ProcessHandler(self.command,
                                   processOutputLine=[self.on_output],
                                   onFinish=self.on_finish)
        self.proc.run()

        timeout = test.timeout * self.timeout_multiplier

        
        self.result_flag.wait(timeout + 5)

        if self.result_flag.is_set() and self.result_data is not None:
            self.result_data["test"] = test.url
            result = self.convert_result(test, self.result_data)
            self.proc.kill()
        else:
            if self.proc.proc.poll() is not None:
                result = (test.result_cls("CRASH", None), [])
            else:
                self.proc.kill()
                result = (test.result_cls("TIMEOUT", None), [])
        self.runner.send_message("test_ended", test, result)

    def on_output(self, line):
        prefix = "ALERT: RESULT: "
        line = line.decode("utf8", "replace")
        if line.startswith(prefix):
            self.result_data = json.loads(line[len(prefix):])
            self.result_flag.set()
        else:
            if self.interactive:
                print line
            else:
                self.logger.process_output(self.proc.pid,
                                           line,
                                           " ".join(self.command))

    def on_finish(self):
        self.result_flag.set()
