






































"""tests for ManifestDestiny"""

import doctest
import os
import sys
from optparse import OptionParser

def run_tests(raise_on_error=False, report_first=False):

    
    results = {}

    
    directory = os.path.dirname(os.path.abspath(__file__))
    extraglobs = {}
    doctest_args = dict(extraglobs=extraglobs,
                        module_relative=False,
                        raise_on_error=raise_on_error)
    if report_first:
        doctest_args['optionflags'] = doctest.REPORT_ONLY_FIRST_FAILURE
                                
    
    directory = os.path.dirname(os.path.abspath(__file__))
    tests =  [ test for test in os.listdir(directory)
               if test.endswith('.txt') and test.startswith('test_')]
    os.chdir(directory)

    
    for test in tests:
        try:
            results[test] = doctest.testfile(test, **doctest_args)
        except doctest.DocTestFailure, failure:
            raise
        except doctest.UnexpectedException, failure:
            raise failure.exc_info[0], failure.exc_info[1], failure.exc_info[2]
        
    return results
                                

def main(args=sys.argv[1:]):

    
    parser = OptionParser(description=__doc__)
    parser.add_option('--raise', dest='raise_on_error',
                      default=False, action='store_true',
                      help="raise on first error")
    parser.add_option('--report-first', dest='report_first',
                      default=False, action='store_true',
                      help="report the first error only (all tests will still run)")
    options, args = parser.parse_args(args)

    
    results = run_tests(**options.__dict__)

    
    failed = False
    for result in results.values():
        if result[0]: 
            failed = True
            break
    if failed:
        sys.exit(1) 
               
if __name__ == '__main__':
    main()
