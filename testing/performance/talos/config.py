




































"""A list of constants containing the paths to programs and files
   needed by the performance testing scripts.
"""
__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'

"""The path to the file url to load when initializing/collecting information from the browser"""
INIT_URL = 'file:///c:/mozilla/testing/performance/talos/getInfo.html'

"""Dump results locally to csv"""
TO_CSV = 0
CSV_FILE = r'c:\mozilla\testing\performance\talos\output\out'
"""URL for the results server"""
TO_GRAPH_SERVER = 1
RESULTS_SERVER = 'graphserver.url.here'
RESULTS_LINK = '/bulk.cgi'

"""Enable/disable debugging output"""
DEBUG = 0
