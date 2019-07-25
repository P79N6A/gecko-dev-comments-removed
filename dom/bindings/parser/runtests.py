




































import os, sys
import glob
import WebIDL

class TestHarness(object):
    def ok(self, condition, msg):
        if condition:
            print "TEST-PASS | %s" % msg
        else:
            print "TEST-UNEXPECTED-FAIL | %s" % msg

    def check(self, a, b, msg):
        if a == b:
            print "TEST-PASS | %s" % msg
        else:
            print "TEST-UNEXPECTED-FAIL | %s" % msg
            print "\tGot %s expected %s" % (a, b)

def run_tests():
    harness = TestHarness()

    tests = glob.iglob("tests/*.py")
    sys.path.append("./tests")
    for test in tests:
        (testpath, ext) = os.path.splitext(os.path.basename(test))
        _test = __import__(testpath, globals(), locals(), ['WebIDLTest'])
        
        _test.WebIDLTest.__call__(WebIDL.Parser(), harness)
        
        
        
        print "Test %s Complete\n" % testpath

if __name__ == '__main__':
    run_tests()
