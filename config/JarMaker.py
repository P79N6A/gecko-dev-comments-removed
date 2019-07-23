




































'''jarmaker.py provides a python class to package up chrome content by
processing jar.mn files.

See the documentation for jar.mn on MDC for further details on the format.
'''

import sys
import os
import os.path
import re
import logging
from time import localtime
from optparse import OptionParser
from MozZipFile import ZipFile
from cStringIO import StringIO
from datetime import datetime

from utils import pushback_iter
from Preprocessor import Preprocessor

__all__ = ['JarMaker']

class ZipEntry:
  '''Helper class for jar output.

  This class defines a simple file-like object for a zipfile.ZipEntry
  so that we can consecutively write to it and then close it.
  This methods hooks into ZipFile.writestr on close().
  '''
  def __init__(self, name, zipfile):
    self._zipfile = zipfile
    self._name = name
    self._inner = StringIO()

  def write(self, content):
    'Append the given content to this zip entry'
    self._inner.write(content)
    return

  def close(self):
    'The close method writes the content back to the zip file.'
    self._zipfile.writestr(self._name, self._inner.getvalue())

def getModTime(aPath):
  if not os.path.isfile(aPath):
    return 0
  mtime = os.stat(aPath).st_mtime
  return localtime(mtime)


class JarMaker(object):
  '''JarMaker reads jar.mn files and process those into jar files or
  flat directories, along with chrome.manifest files.
  '''

  ignore = re.compile('\s*(\#.*)?$')
  jarline = re.compile('(?:(?P<jarfile>[\w\d.\-\_\\\/]+).jar\:)|(?:\s*(\#.*)?)\s*$')
  regline = re.compile('\%\s+(.*)$')
  entryre = '(?P<optPreprocess>\*)?(?P<optOverwrite>\+?)\s+'
  entryline = re.compile(entryre + '(?P<output>[\w\d.\-\_\\\/\+]+)\s*(\((?P<locale>\%?)(?P<source>[\w\d.\-\_\\\/]+)\))?\s*$')

  def __init__(self, outputFormat = 'flat', useJarfileManifest = True,
               useChromeManifest = False):
    self.outputFormat = outputFormat
    self.useJarfileManifest = useJarfileManifest
    self.useChromeManifest = useChromeManifest
    self.pp = Preprocessor()

  def getCommandLineParser(self):
    '''Get a optparse.OptionParser for jarmaker.

    This OptionParser has the options for jarmaker as well as
    the options for the inner PreProcessor.
    '''
    
    
    p = self.pp.getCommandLineParser(unescapeDefines = True)
    p.add_option('-f', type="choice", default="jar",
                 choices=('jar', 'flat', 'symlink'),
                 help="fileformat used for output", metavar="[jar, flat, symlink]")
    p.add_option('-v', action="store_true", dest="verbose",
                 help="verbose output")
    p.add_option('-q', action="store_false", dest="verbose",
                 help="verbose output")
    p.add_option('-e', action="store_true",
                 help="create chrome.manifest instead of jarfile.manifest")
    p.add_option('--both-manifests', action="store_true",
                 dest="bothManifests",
                 help="create chrome.manifest and jarfile.manifest")
    p.add_option('-s', type="string", action="append", default=[],
                 help="source directory")
    p.add_option('-t', type="string",
                 help="top source directory")
    p.add_option('-c', '--l10n-src', type="string", action="append",
                 help="localization directory")
    p.add_option('--l10n-base', type="string", action="append", default=[],
                 help="base directory to be used for localization (multiple)")
    p.add_option('-j', type="string",
                 help="jarfile directory")
    
    p.add_option('-a', action="store_false", default=True,
                 help="NOT SUPPORTED, turn auto-registration of chrome off (installed-chrome.txt)")
    p.add_option('-d', type="string",
                 help="UNUSED, chrome directory")
    p.add_option('-o', help="cross compile for auto-registration, ignored")
    p.add_option('-l', action="store_true",
                 help="ignored (used to switch off locks)")
    p.add_option('-x', action="store_true",
                 help="force Unix")
    p.add_option('-z', help="backwards compat, ignored")
    p.add_option('-p', help="backwards compat, ignored")
    return p

  def processIncludes(self, includes):
    '''Process given includes with the inner PreProcessor.

    Only use this for #defines, the includes shouldn't generate
    content.
    '''
    self.pp.out = StringIO()
    for inc in includes:
      self.pp.do_include(inc)
    includesvalue = self.pp.out.getvalue()
    if includesvalue:
      logging.info("WARNING: Includes produce non-empty output")
    self.pp.out = None
    pass

  def finalizeJar(self, jarPath, chromebasepath, register,
                   doZip=True):
    '''Helper method to write out the chrome registration entries to
    jarfile.manifest or chrome.manifest, or both.

    The actual file processing is done in updateManifest.
    '''
    
    if not register:
      return
    if self.useJarfileManifest:
      self.updateManifest(jarPath + '.manifest', chromebasepath % '',
                          register)
    if self.useChromeManifest:
      manifestPath = os.path.join(os.path.dirname(jarPath),
                                  '..', 'chrome.manifest')
      self.updateManifest(manifestPath, chromebasepath % 'chrome/',
                          register)

  def updateManifest(self, manifestPath, chromebasepath, register):
    '''updateManifest replaces the % in the chrome registration entries
    with the given chrome base path, and updates the given manifest file.
    '''
    myregister = dict.fromkeys(map(lambda s: s.replace('%', chromebasepath),
                                   register.iterkeys()))
    manifestExists = os.path.isfile(manifestPath)
    mode = (manifestExists and 'r+b') or 'wb'
    mf = open(manifestPath, mode)
    if manifestExists:
      
      imf = re.compile('(#.*)?$')
      for l in re.split('[\r\n]+', mf.read()):
        if imf.match(l):
          continue
        myregister[l] = None
      mf.seek(0)
    for k in myregister.iterkeys():
      mf.write(k + os.linesep)
    mf.close()
  
  def makeJar(self, infile=None,
               jardir='',
               sourcedirs=[], topsourcedir='', localedirs=None):
    '''makeJar is the main entry point to JarMaker.

    It takes the input file, the output directory, the source dirs and the
    top source dir as argument, and optionally the l10n dirs.
    '''
    if isinstance(infile, basestring):
      logging.info("processing " + infile)
    pp = self.pp.clone()
    pp.out = StringIO()
    pp.do_include(infile)
    lines = pushback_iter(pp.out.getvalue().splitlines())
    try:
      while True:
        l = lines.next()
        m = self.jarline.match(l)
        if not m:
          raise RuntimeError(l)
        if m.group('jarfile') is None:
          
          continue
        self.processJarSection(m.group('jarfile'), lines,
                               jardir, sourcedirs, topsourcedir,
                               localedirs)
    except StopIteration:
      
      pass
    return

  def processJarSection(self, jarfile, lines,
                        jardir, sourcedirs, topsourcedir, localedirs):
    '''Internal method called by makeJar to actually process a section
    of a jar.mn file.

    jarfile is the basename of the jarfile or the directory name for 
    flat output, lines is a pushback_iterator of the lines of jar.mn,
    the remaining options are carried over from makeJar.
    '''

    
    
    
    chromebasepath = '%s' + jarfile
    if self.outputFormat == 'jar':
      chromebasepath = 'jar:' + chromebasepath + '.jar!'
    chromebasepath += '/'

    jarfile = os.path.join(jardir, jarfile)
    jf = None
    if self.outputFormat == 'jar':
      
      jarfilepath = jarfile + '.jar'
      try:
        os.makedirs(os.path.dirname(jarfilepath))
      except OSError:
        pass
      jf = ZipFile(jarfilepath, 'a', lock = True)
      outHelper = self.OutputHelper_jar(jf)
    else:
      outHelper = getattr(self, 'OutputHelper_' + self.outputFormat)(jarfile)
    register = {}
    
    
    
    
    try:
      while True:
        try:
          l = lines.next()
        except StopIteration:
          
          self.finalizeJar(jarfile, chromebasepath, register)
          if jf is not None:
            jf.close()
          
          raise
        if self.ignore.match(l):
          continue
        m = self.regline.match(l)
        if  m:
          rline = m.group(1)
          register[rline] = 1
          continue
        m = self.entryline.match(l)
        if not m:
          
          self.finalizeJar(jarfile, chromebasepath, register)
          if jf is not None:
            jf.close()
          lines.pushback(l)
          return
        self._processEntryLine(m, sourcedirs, topsourcedir, localedirs,
                              outHelper, jf)
    finally:
      if jf is not None:
        jf.close()
    return

  def _processEntryLine(self, m, 
                        sourcedirs, topsourcedir, localedirs,
                        outHelper, jf):
      out = m.group('output')
      src = m.group('source') or os.path.basename(out)
      
      if m.group('locale'):
        src_base = localedirs
      elif src.startswith('/'):
        
        
        
        src_base = [topsourcedir]
        src = src[1:]
      else:
        
        src_base = sourcedirs + ['.']
      
      realsrc = None
      for _srcdir in src_base:
        if os.path.isfile(os.path.join(_srcdir, src)):
          realsrc = os.path.join(_srcdir, src)
          break
      if realsrc is None:
        if jf is not None:
          jf.close()
        raise RuntimeError("file not found: " + src)
      if m.group('optPreprocess'):
        outf = outHelper.getOutput(out)
        inf = open(realsrc)
        pp = self.pp.clone()
        if src[-4:] == '.css':
          pp.setMarker('%')
        pp.out = outf
        pp.do_include(inf)
        outf.close()
        inf.close()
        return
      
      if (m.group('optOverwrite')
          or (getModTime(realsrc) >
              outHelper.getDestModTime(m.group('output')))):
        if self.outputFormat == 'symlink' and hasattr(os, 'symlink'):
          outHelper.symlink(realsrc, out)
          return
        outf = outHelper.getOutput(out)
        
        inf = open(realsrc, 'rb')
        outf.write(inf.read())
        outf.close()
        inf.close()
    

  class OutputHelper_jar(object):
    '''Provide getDestModTime and getOutput for a given jarfile.
    '''
    def __init__(self, jarfile):
      self.jarfile = jarfile
    def getDestModTime(self, aPath):
      try :
        info = self.jarfile.getinfo(aPath)
        return info.date_time
      except:
        return 0
    def getOutput(self, name):
      return ZipEntry(name, self.jarfile)

  class OutputHelper_flat(object):
    '''Provide getDestModTime and getOutput for a given flat
    output directory. The helper method ensureDirFor is used by
    the symlink subclass.
    '''
    def __init__(self, basepath):
      self.basepath = basepath
    def getDestModTime(self, aPath):
      return getModTime(os.path.join(self.basepath, aPath))
    def getOutput(self, name):
      out = self.ensureDirFor(name)
      
      try:
        os.remove(out)
      except OSError, e:
        if e.errno != 2:
          raise
      return open(out, 'wb')
    def ensureDirFor(self, name):
      out = os.path.join(self.basepath, name)
      outdir = os.path.dirname(out)
      if not os.path.isdir(outdir):
        os.makedirs(outdir)
      return out

  class OutputHelper_symlink(OutputHelper_flat):
    '''Subclass of OutputHelper_flat that provides a helper for
    creating a symlink including creating the parent directories.
    '''
    def symlink(self, src, dest):
      out = self.ensureDirFor(dest)
      
      try:
        os.remove(out)
      except OSError, e:
        if e.errno != 2:
          raise
      os.symlink(src, out)

def main():
  jm = JarMaker()
  p = jm.getCommandLineParser()
  (options, args) = p.parse_args()
  jm.processIncludes(options.I)
  jm.outputFormat = options.f
  if options.e:
    jm.useChromeManifest = True
    jm.useJarfileManifest = False
  if options.bothManifests:
    jm.useChromeManifest = True
    jm.useJarfileManifest = True
  noise = logging.INFO
  if options.verbose is not None:
    noise = (options.verbose and logging.DEBUG) or logging.WARN
  if sys.version_info[:2] > (2,3):
    logging.basicConfig(format = "%(message)s")
  else:
    logging.basicConfig()
  logging.getLogger().setLevel(noise)
  topsrc = options.t
  topsrc = os.path.normpath(os.path.abspath(topsrc))
  if not args:
    jm.makeJar(infile=sys.stdin,
               sourcedirs=options.s, topsourcedir=topsrc,
               localedirs=options.l10n_src,
               jardir=options.j)
    return
  for infile in args:
    
    
    
    
    srcdir = os.path.normpath(os.path.abspath(os.path.dirname(infile)))
    l10ndir = srcdir
    if os.path.basename(srcdir) == 'locales':
      l10ndir = os.path.dirname(l10ndir)
    assert srcdir.startswith(topsrc), "src dir %s not in topsrcdir %s" % (srcdir, topsrc)
    rell10ndir = l10ndir[len(topsrc):].lstrip(os.sep)
    l10ndirs = map(lambda d: os.path.join(d, rell10ndir), options.l10n_base)
    if options.l10n_src is not None:
      l10ndirs += map(lambda s: os.path.normpath(os.path.abspath(s)),
                      options.l10n_src)
    srcdirs = map(lambda s: os.path.normpath(os.path.abspath(s)), options.s) + \
        [srcdir]
    jm.makeJar(infile=infile,
               sourcedirs=srcdirs, topsourcedir=topsrc,
               localedirs=l10ndirs,
               jardir=options.j)

if __name__ == "__main__":
  main()
