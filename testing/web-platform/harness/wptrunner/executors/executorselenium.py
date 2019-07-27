



import os
import socket
import sys
import threading
import time
import traceback
import urlparse
import uuid

from .base import TestExecutor, testharness_result_converter
from ..testrunner import Stop


here = os.path.join(os.path.split(__file__)[0])

webdriver = None
exceptions = None

required_files = [("testharness_runner.html", "", False),
                  ("testharnessreport.js", "resources/", True)]


def do_delayed_imports():
    global webdriver
    global exceptions
    from selenium import webdriver
    from selenium.common import exceptions


class SeleniumTestExecutor(TestExecutor):
    def __init__(self, browser, http_server_url, timeout_multiplier=1,
                 **kwargs):
        do_delayed_imports()
        TestExecutor.__init__(self, browser, http_server_url, timeout_multiplier)
        self.webdriver_port = browser.webdriver_port
        self.webdriver = None

        self.timer = None
        self.window_id = str(uuid.uuid4())
        self.capabilities = kwargs.pop("capabilities")

    def setup(self, runner):
        """Connect to browser via Selenium's WebDriver implementation."""
        self.runner = runner
        url = "http://localhost:%i/wd/url" % self.webdriver_port
        self.logger.debug("Connecting to Selenium on URL: %s" % url)

        session_started = False
        try:
            time.sleep(1)
            self.webdriver = webdriver.Remote(
                url, desired_capabilities=self.capabilities)
            time.sleep(10)
        except:
            self.logger.warning(
                "Connecting to Selenium failed:\n%s" % traceback.format_exc())
            time.sleep(1)
        else:
            self.logger.debug("Selenium session started")
            session_started = True

        if not session_started:
            self.logger.warning("Failed to connect to Selenium")
            self.runner.send_message("init_failed")
        else:
            try:
                self.after_connect()
            except:
                print >> sys.stderr, traceback.format_exc()
                self.logger.warning(
                    "Failed to connect to navigate initial page")
                self.runner.send_message("init_failed")
            else:
                self.runner.send_message("init_succeeded")

    def teardown(self):
        try:
            self.webdriver.quit()
        except:
            pass
        del self.webdriver

    def is_alive(self):
        try:
            
            self.webdriver.current_window_handle
        
        except (socket.timeout, exceptions.ErrorInResponseException):
            return False
        return True

    def after_connect(self):
        url = urlparse.urljoin(self.http_server_url, "/testharness_runner.html")
        self.logger.debug("Loading %s" % url)
        self.webdriver.get(url)
        self.webdriver.execute_script("document.title = '%s'" %
                                      threading.current_thread().name.replace("'", '"'))

    def run_test(self, test):
        """Run a single test.

        This method is independent of the test type, and calls
        do_test to implement the type-sepcific testing functionality.
        """
        
        
        
        
        result = None
        result_flag = threading.Event()
        result_lock = threading.Lock()

        timeout = test.timeout * self.timeout_multiplier

        def timeout_func():
            with result_lock:
                if not result_flag.is_set():
                    result_flag.set()
                    result = (test.result_cls("EXTERNAL-TIMEOUT", None), [])
                    self.runner.send_message("test_ended", test, result)

        self.timer = threading.Timer(timeout + 10, timeout_func)
        self.timer.start()

        try:
            self.webdriver.set_script_timeout((timeout + 5) * 1000)
        except exceptions.ErrorInResponseException:
            self.logger.error("Lost webdriver connection")
            self.runner.send_message("restart_test", test)
            return Stop

        try:
            result = self.convert_result(test, self.do_test(test, timeout))
        except exceptions.TimeoutException:
            with result_lock:
                if not result_flag.is_set():
                    result_flag.set()
                    result = (test.result_cls("EXTERNAL-TIMEOUT", None), [])
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

        
        except (socket.timeout, exceptions.ErrorInResponseException):
            
            
            
            with result_lock:
                if not result_flag.is_set():
                    result_flag.set()
                    result = (test.result_cls("CRASH", None), [])
        finally:
            self.timer.cancel()

        with result_lock:
            if result:
                self.runner.send_message("test_ended", test, result)


class SeleniumTestharnessExecutor(SeleniumTestExecutor):
    convert_result = testharness_result_converter

    def __init__(self, *args, **kwargs):
        SeleniumTestExecutor.__init__(self, *args, **kwargs)
        self.script = open(os.path.join(here, "testharness_webdriver.js")).read()

    def do_test(self, test, timeout):
        return self.webdriver.execute_async_script(
            self.script % {"abs_url": urlparse.urljoin(self.http_server_url, test.url),
                           "url": test.url,
                           "window_id": self.window_id,
                           "timeout_multiplier": self.timeout_multiplier,
                           "timeout": timeout * 1000})
