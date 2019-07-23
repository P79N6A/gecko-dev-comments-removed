





































"""A list of constants containing the paths to programs and files
   needed by the performance testing scripts.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


"""For some reason, can only get output from dump() in Firefox if
   it's run through cygwin bash.  So here's the path to cygwin.
"""
CYGWIN = r'c:\cygwin\bin\bash.exe -c'

"""The tinderbox scripts run sync between Ts runs, so we do, too."""
SYNC = r'c:\cygwin\bin\sync'

"""The path to the base profile directory to use for testing.  For the page
   load test to work, this profile should have its hostperm.1 file set to allow
   urls with scheme:file to open in new windows, and the preference to open
   new windows in a tab should be off.
"""
BASE_PROFILE_DIR = r'C:\extension_perf_testing\base_profile'

"""The directory the generated reports go into."""
REPORTS_DIR = r'c:\extension_perf_reports'

"""The path to the file url to load when initializing a new profile"""
INIT_URL = 'file:///c:/mozilla/testing/performance/win32/initialize.html'

"""The path to the file url to load for startup test (Ts)"""
TS_URL = 'file:///c:/mozilla/testing/performance/win32/startup_test/startup_test.html?begin='

"""The path to the file url to load for page load test (Tp)"""
TP_URL = 'file:///c:/mozilla/testing/performance/win32/page_load_test/cycler.html'
