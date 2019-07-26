



"""Defines a set of constants shared by test runners and other scripts."""

import os


CHROME_PACKAGE = 'com.google.android.apps.chrome'
CHROME_ACTIVITY = 'com.google.android.apps.chrome.Main'
CHROME_TESTS_PACKAGE = 'com.google.android.apps.chrome.tests'
LEGACY_BROWSER_PACKAGE = 'com.google.android.browser'
LEGACY_BROWSER_ACTIVITY = 'com.android.browser.BrowserActivity'
CONTENT_SHELL_PACKAGE = "org.chromium.content_shell"
CONTENT_SHELL_ACTIVITY = "org.chromium.content_shell.ContentShellActivity"
CHROME_SHELL_PACKAGE = 'org.chromium.chrome.browser.test'
CHROMIUM_TEST_SHELL_PACKAGE = 'org.chromium.chrome.testshell'

CHROME_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                          '..', '..', '..'))




LIGHTTPD_DEFAULT_PORT = 9000
LIGHTTPD_RANDOM_PORT_FIRST = 8001
LIGHTTPD_RANDOM_PORT_LAST = 8999
TEST_SYNC_SERVER_PORT = 9031




TEST_SERVER_PORT_FIRST = 10000
TEST_SERVER_PORT_LAST = 30000

TEST_SERVER_PORT_FILE = '/tmp/test_server_port'
TEST_SERVER_PORT_LOCKFILE = '/tmp/test_server_port.lock'

TEST_EXECUTABLE_DIR = '/data/local/tmp'


SDK_BUILD_TEST_JAVALIB_DIR = 'test.lib.java'
SDK_BUILD_APKS_DIR = 'apks'


DEVICE_PERF_OUTPUT_DIR = '/data/data/' + CHROME_PACKAGE + '/files'
