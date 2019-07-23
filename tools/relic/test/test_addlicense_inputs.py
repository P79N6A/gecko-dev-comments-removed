



"""Test adding licenses to addlicense_inputs/... with relic.py."""

import sys
import os
import unittest
import difflib
import pprint
import shutil
import StringIO

import testsupport



gInputsDir = "addlicense_inputs"
gOutputsDir = "addlicense_outputs"
gTmpDir = "addlicense_tmp"




class RelicInputsTestCase(unittest.TestCase):
    def setUp(self):
        if not os.path.exists(gTmpDir):
            os.mkdir(gTmpDir)

    def tearDown(self):
        testsupport.rmtree(gTmpDir)


def _testOneInputFile(self, fname):
    import relic
    _debug = 0  

    infile = os.path.join(gInputsDir, fname) 
    outfile = os.path.join(gOutputsDir, fname) 
    tmpfile = os.path.join(gTmpDir, fname) 
    errfile = os.path.join(gOutputsDir, fname+'.error')  
    
    
    
    
    optsfile = os.path.join(gInputsDir, fname+'.options') 

    if _debug:
        print
        print "*"*50, "relic '%s'" % fname

    
    opts = {}
    if os.path.exists(optsfile):
        for line in open(optsfile, 'r').read().splitlines(0):
            name, value = line.split('=', 1)
            value = value.strip()
            try: 
                value = eval(value)
            except Exception:
                pass
            opts[name] = value
        if _debug:
            print "*"*50, "options"
            pprint.pprint(opts)
    else:
        
        
        opts = {
            "original_code_is": "mozilla.org Code",
            "initial_copyright_date": "2001",
            "initial_developer": "Netscape Communications Corporation",
        }

    
    shutil.copy(infile, tmpfile)

    
    
    oldStdout = sys.stdout
    oldStderr = sys.stderr
    sys.stdout = StringIO.StringIO()
    sys.stderr = StringIO.StringIO()
    try:
        try:
            relic.addlicense([tmpfile], **opts)
        except relic.RelicError, ex:
            error = ex
        else:
            error = None
    finally:
        stdout = sys.stdout.getvalue()
        stderr = sys.stderr.getvalue()
        sys.stdout = oldStdout
        sys.stderr = oldStderr
    if _debug:
        print "*"*50, "stdout"
        print stdout
        print "*"*50, "stderr"
        print stderr
        print "*"*50, "error"
        print str(error)
        print "*" * 50

    
    if os.path.exists(outfile) and error:
        self.fail("adding license '%s' raised an error but success was "
                  "expected: error='%s'" % (fname, str(error)))
    elif os.path.exists(outfile):
        expected = open(outfile, 'r').readlines()
        actual = open(tmpfile, 'r').readlines()
        if expected != actual:
            diff = list(difflib.ndiff(expected, actual))
            self.fail("%r != %r:\n%s"\
                      % (outfile, tmpfile, pprint.pformat(diff)))
    elif os.path.exists(errfile):
        
        
        expectedError = open(errfile, 'r').read()
        actualError = str(error)
        self.failUnlessEqual(actualError.strip(), expectedError.strip())
    else:
        self.fail("No reference ouput file or error file for '%s'." % infile)

    
    del sys.modules['relic']
        


for fname in os.listdir(gInputsDir):
    if fname.endswith(".options"): continue 
    testFunction = lambda self, fname=fname: _testOneInputFile(self, fname)
    name = 'test_addlicense_'+fname
    setattr(RelicInputsTestCase, name, testFunction)




def suite():
    """Return a unittest.TestSuite to be used by test.py."""
    return unittest.makeSuite(RelicInputsTestCase)

if __name__ == "__main__":
    runner = unittest.TextTestRunner(sys.stdout, verbosity=2)
    result = runner.run(suite())

