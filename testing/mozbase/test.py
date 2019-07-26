





"""
run mozbase tests from a manifest,
by default https://github.com/mozilla/mozbase/blob/master/test-manifest.ini
"""

import imp
import manifestparser
import os
import sys
import unittest

from moztest.results import TestResultCollection

here = os.path.dirname(os.path.abspath(__file__))

def unittests(path):
    """return the unittests in a .py file"""

    path = os.path.abspath(path)
    unittests = []
    assert os.path.exists(path)
    directory = os.path.dirname(path)
    sys.path.insert(0, directory) 
    modname = os.path.splitext(os.path.basename(path))[0]
    module = imp.load_source(modname, path)
    sys.path.pop(0) 
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromModule(module)
    for test in suite:
        unittests.append(test)
    return unittests

def main(args=sys.argv[1:]):

    
    if args:
        manifests = args
    else:
        manifests = [os.path.join(here, 'test-manifest.ini')]
    missing = []
    for manifest in manifests:
        
        if not os.path.exists(manifest):
            missing.append(manifest)
    assert not missing, 'manifest%s not found: %s' % ((len(manifests) == 1 and '' or 's'), ', '.join(missing))
    manifest = manifestparser.TestManifest(manifests=manifests)

    
    tests = manifest.active_tests()
    unittestlist = []
    for test in tests:
        unittestlist.extend(unittests(test['path']))

    
    suite = unittest.TestSuite(unittestlist)
    runner = unittest.TextTestRunner(verbosity=2) 
    unittest_results = runner.run(suite)
    results = TestResultCollection.from_unittest_results(None, unittest_results)

    
    sys.exit(1 if results.num_failures else 0)

if __name__ == '__main__':
    main()
