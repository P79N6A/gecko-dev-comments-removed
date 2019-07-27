




import unittest
import os.path

parent = os.path.dirname
test_dir = parent(os.path.abspath(__file__))
sdk_root = parent(parent(parent(test_dir)))

def from_sdk_top(fn):
    return os.path.abspath(os.path.join(sdk_root, fn))

MPL2_URL = "http://mozilla.org/MPL/2.0/"


skip = [
    "python-lib/cuddlefish/_version.py", 
    "doc/static-files/js/jquery.js", 
    "examples/annotator/data/jquery-1.4.2.min.js", 
    "examples/reddit-panel/data/jquery-1.4.4.min.js", 
    "examples/library-detector/data/library-detector.js", 
    "python-lib/mozrunner/killableprocess.py", 
    "python-lib/mozrunner/winprocess.py", 
    "packages/api-utils/tests/test-querystring.js", 
    "packages/api-utils/lib/promise.js", 
    "packages/api-utils/tests/test-promise.js", 
    "examples/actor-repl/README.md", 
    "examples/actor-repl/data/codemirror-compressed.js", 
    "examples/actor-repl/data/codemirror.css", 
    ]
absskip = [from_sdk_top(os.path.join(*fn.split("/"))) for fn in skip]

class Licenses(unittest.TestCase):
    def test(self):
        
        
        
        self.missing = []
        self.scan_file(from_sdk_top(os.path.join("python-lib", "jetpack_sdk_env.py")))
        self.scan(os.path.join("python-lib", "cuddlefish"), [".js", ".py"],
                  skipdirs=["sdk-docs"], 
                  )
        self.scan(os.path.join("python-lib", "mozrunner"), [".py"])
        self.scan("lib", [".js", ".jsm", ".css"],
                  skipdirs=[
                    "diffpatcher", 
                    "method", 
                    "child_process", 
                    "fs" 
                  ])
        self.scan("test", [".js", ".jsm", ".css", ".html"],
                  skipdirs=[
                    "buffers", 
                    "querystring", 
                    "path" 
                  ])
        self.scan("modules", [".js", ".jsm"])
        self.scan("examples", [".js", ".css", ".html", ".md"])
        self.scan("bin", [".bat", ".ps1", ".js"])
        for fn in [os.path.join("bin", "activate"),
                   os.path.join("bin", "cfx"),
                   os.path.join("bin", "integration-scripts", "buildbot-run-cfx-helper"),
                   os.path.join("bin", "integration-scripts", "integration-check"),
                   ]:
            self.scan_file(from_sdk_top(fn))

        if self.missing:
            print
            print "The following files are missing an MPL2 header:"
            for fn in sorted(self.missing):
                print " "+fn
            self.fail("%d files are missing an MPL2 header" % len(self.missing))

    def scan(self, start, extensions=[], skipdirs=[]):
        
        start = from_sdk_top(start)
        for root, dirs, files in os.walk(start):
            for d in skipdirs:
                if d in dirs:
                    dirs.remove(d)
            for fn in files:
                ext = os.path.splitext(fn)[1]
                if extensions and ext not in extensions:
                    continue
                absfn = os.path.join(root, fn)
                if absfn in absskip:
                    continue
                self.scan_file(absfn)

    def scan_file(self, fn):
        
        if not MPL2_URL in open(fn, "r").read():
            relfile = fn[len(sdk_root)+1:]
            self.missing.append(relfile)

if __name__ == '__main__':
    unittest.main()
