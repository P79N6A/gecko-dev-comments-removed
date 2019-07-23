




































"""Implementation of Python file objects for Mozilla/xpcom.

Introduction:
  This module defines various class that are implemented using 
  Mozilla streams.  This allows you to open Mozilla URI's, and
  treat them as Python file object.
  
Example:
>>> file = URIFile("chrome://whatever")
>>> data = file.read(5) # Pass no arg to read everything.

Known Limitations:
 * Not all URL schemes will work from "python.exe" - most notably
   "chrome://" and "http://" URLs - this is because a simple initialization of
   xpcom by Python does not load up the full set of Mozilla URL handlers.
   If you can work out how to correctly initialize the chrome registry and
   setup a message queue.

Known Bugs:
  * Only read ("r") mode is supported.  Although write ("w") mode doesn't make
    sense for HTTP type URLs, it potentially does for file:// etc type ones.
  * No concept of text mode vs binary mode.  It appears Mozilla takes care of
    this internally (ie, all "text/???" mime types are text, rest are binary)

"""

from xpcom import components, Exception, _xpcom
import os
import threading 

NS_RDONLY        = 0x01
NS_WRONLY        = 0x02
NS_RDWR          = 0x04
NS_CREATE_FILE   = 0x08
NS_APPEND        = 0x10
NS_TRUNCATE      = 0x20
NS_SYNC          = 0x40
NS_EXCL          = 0x80


def LocalFileToURL(localFileName):
    "Convert a filename to an XPCOM nsIFileURL object."
    
    localFile = components.classes["@mozilla.org/file/local;1"] \
          .createInstance(components.interfaces.nsILocalFile)
    localFile.initWithPath(localFileName)

    
    io_service = components.classes["@mozilla.org/network/io-service;1"] \
                    .getService(components.interfaces.nsIIOService)
    url = io_service.newFileURI(localFile).queryInterface(components.interfaces.nsIFileURL)
    
    url.file = localFile
    return url


class _File:
    def __init__(self, name_thingy = None, mode="r"):
        self.lockob = threading.Lock()
        self.inputStream = self.outputStream = None
        if name_thingy is not None:
                self.init(name_thingy, mode)

    def __del__(self):
        self.close()

    
    def _lock(self):
        self.lockob.acquire()
    def _release(self):
        self.lockob.release()
    def read(self, n = -1):
        assert self.inputStream is not None, "Not setup for read!"
        self._lock()
        try:
            return str(self.inputStream.read(n))
        finally:
            self._release()

    def readlines(self):
        
        
        lines = self.read().split("\n")
        if len(lines) and len(lines[-1]) == 0:
            lines = lines[:-1]
        return [s+"\n" for s in lines ]

    def write(self, data):
        assert self.outputStream is not None, "Not setup for write!"
        self._lock()
        try:
            self.outputStream.write(data, len(data))
        finally:
            self._release()

    def close(self):
        self._lock()
        try:
            if self.inputStream is not None:
                self.inputStream.close()
                self.inputStream = None
            if self.outputStream is not None:
                self.outputStream.close()
                self.outputStream = None
            self.channel = None
        finally:
            self._release()

    def flush(self):
        self._lock()
        try:
            if self.outputStream is not None: self.outputStream.flush()
        finally:
            self._release()


class URIFile(_File):
    def init(self, url, mode="r"):
        self.close()
        if mode != "r":
            raise ValueError, "only 'r' mode supported'"
        io_service = components.classes["@mozilla.org/network/io-service;1"] \
                        .getService(components.interfaces.nsIIOService)
        if hasattr(url, "queryInterface"):
            url_ob = url
        else:
            url_ob = io_service.newURI(url, None, None)
        
        if not url_ob.scheme:
            raise ValueError, ("The URI '%s' is invalid (no scheme)" 
                                  % (url_ob.spec,))
        self.channel = io_service.newChannelFromURI(url_ob)
        self.inputStream = self.channel.open()





class LocalFile(_File):
    def __init__(self, *args):
        self.fileIO = None
        _File.__init__(self, *args)

    def init(self, name, mode = "r"):
        name = os.path.abspath(name) 
        self.close()
        file = components.classes['@mozilla.org/file/local;1'].createInstance("nsILocalFile")
        file.initWithPath(name)
        if mode in ["w","a"]:
            self.fileIO = components.classes["@mozilla.org/network/file-output-stream;1"].createInstance("nsIFileOutputStream")
            if mode== "w":
                if file.exists():
                    file.remove(0)
                moz_mode = NS_CREATE_FILE | NS_WRONLY
            elif mode=="a":
                moz_mode = NS_APPEND
            else:
                assert 0, "Can't happen!"
            self.fileIO.init(file, moz_mode, -1,0)
            self.outputStream = self.fileIO
        elif mode == "r":
            self.fileIO = components.classes["@mozilla.org/network/file-input-stream;1"].createInstance("nsIFileInputStream")
            self.fileIO.init(file, NS_RDONLY, -1,0)
            self.inputStream = components.classes["@mozilla.org/scriptableinputstream;1"].createInstance("nsIScriptableInputStream")
            self.inputStream.init(self.fileIO)
        else:
            raise ValueError, "Unknown mode"

    def close(self):
        if self.fileIO is not None:
            self.fileIO.close()
            self.fileIO = None
        _File.close(self)

    def read(self, n = -1):
        return _File.read(self, n)







def _DoTestRead(file, expected):
    
    got = file.read(3)
    got = got + file.read(300)
    got = got + file.read(0)
    got = got + file.read()
    if got != expected:
        raise RuntimeError, "Reading '%s' failed - got %d bytes, but expected %d bytes" % (file, len(got), len(expected))

def _DoTestBufferRead(file, expected):
    
    buffer = _xpcom.AllocateBuffer(50)
    got = ''
    while 1:
        
        
        num = file.inputStream.read(buffer)
        if num == 0:
            break
        got = got + str(buffer[:num])
    if got != expected:
        raise RuntimeError, "Reading '%s' failed - got %d bytes, but expected %d bytes" % (file, len(got), len(expected))

def _TestLocalFile():
    import tempfile, os
    fname = tempfile.mktemp()
    data = "Hello from Python"
    test_file = LocalFile(fname, "w")
    try:
        test_file.write(data)
        test_file.close()
        
        f = open(fname, "r")
        assert f.read() == data, "Eeek - Python could not read the data back correctly!"
        f.close()
        
        test_file.init(fname, "r")
        got = str(test_file.read())
        assert got == data, got
        test_file.close()
        
        test_file = LocalFile(fname, "r")
        got = test_file.read(10) + test_file.read()
        assert got == data, got
        test_file.close()
        
        if not os.path.isfile(fname):
            raise RuntimeError, "The file '%s' does not exist, but we are explicitly testing create semantics when it does" % (fname,)
        test_file = LocalFile(fname, "w")
        test_file.write(data)
        test_file.close()
        
        f = open(fname, "r")
        assert f.read() == data, "Eeek - Python could not read the data back correctly after recreating an existing file!"
        f.close()

        
    finally:
        os.unlink(fname)

def _TestAll():
    
    
    
    
    fname = components.__file__
    if fname[-1] in "cCoO": 
            fname = fname[:-1]
    expected = open(fname, "rb").read()
    
    url = LocalFileToURL(fname)
    
    _DoTestRead( URIFile( url.spec), expected)
    
    _DoTestRead( URIFile( url ), expected)

    _DoTestBufferRead( URIFile( url ), expected)

    
    _DoTestRead( LocalFile(fname), expected )

    
    _TestLocalFile()

def _TestURI(url):
    test_file = URIFile(url)
    print "Opened file is", test_file
    got = test_file.read()
    print "Read %d bytes of data from %r" % (len(got), url)
    test_file.close()

if __name__=='__main__':
    import sys
    if len(sys.argv) < 2:
        print "No URL specified on command line - performing self-test"
        _TestAll()
    else:
        _TestURI(sys.argv[1])
