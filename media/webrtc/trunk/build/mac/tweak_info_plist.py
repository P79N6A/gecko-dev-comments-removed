





















import optparse
import os
from os import environ as env
import plistlib
import re
import subprocess
import sys
import tempfile

TOP = os.path.join(env['SRCROOT'], '..')

sys.path.insert(0, os.path.join(TOP, "build/util"))
import lastchange


def _GetOutput(args):
  """Runs a subprocess and waits for termination. Returns (stdout, returncode)
  of the process. stderr is attached to the parent."""
  proc = subprocess.Popen(args, stdout=subprocess.PIPE)
  (stdout, stderr) = proc.communicate()
  return (stdout, proc.returncode)


def _GetOutputNoError(args):
  """Similar to _GetOutput() but ignores stderr. If there's an error launching
  the child (like file not found), the exception will be caught and (None, 1)
  will be returned to mimic quiet failure."""
  try:
    proc = subprocess.Popen(args, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
  except OSError:
    return (None, 1)
  (stdout, stderr) = proc.communicate()
  return (stdout, proc.returncode)


def _RemoveKeys(plist, *keys):
  """Removes a varargs of keys from the plist."""
  for key in keys:
    try:
      del plist[key]
    except KeyError:
      pass


def _AddVersionKeys(plist):
  """Adds the product version number into the plist. Returns True on success and
  False on error. The error will be printed to stderr."""
  
  VERSION_TOOL = os.path.join(TOP, 'chrome/tools/build/version.py')
  VERSION_FILE = os.path.join(TOP, 'chrome/VERSION')

  (stdout, retval1) = _GetOutput([VERSION_TOOL, '-f', VERSION_FILE, '-t',
                                  '@MAJOR@.@MINOR@.@BUILD@.@PATCH@'])
  full_version = stdout.rstrip()

  (stdout, retval2) = _GetOutput([VERSION_TOOL, '-f', VERSION_FILE, '-t',
                                  '@BUILD@.@PATCH@'])
  bundle_version = stdout.rstrip()

  
  
  if retval1 or retval2:
    return False

  
  plist['CFBundleShortVersionString'] = full_version

  
  
  
  
  
  
  
  plist['CFBundleVersion'] = bundle_version

  
  return True


def _DoSCMKeys(plist, add_keys):
  """Adds the SCM information, visible in about:version, to property list. If
  |add_keys| is True, it will insert the keys, otherwise it will remove them."""
  scm_path, scm_revision = None, None
  if add_keys:
    version_info = lastchange.FetchVersionInfo(
        default_lastchange=None, directory=TOP)
    scm_path, scm_revision = version_info.url, version_info.revision

  
  _RemoveKeys(plist, 'SCMRevision')
  if scm_revision != None:
    plist['SCMRevision'] = scm_revision
  elif add_keys:
    print >>sys.stderr, 'Could not determine SCM revision.  This may be OK.'

  if scm_path != None:
    plist['SCMPath'] = scm_path
  else:
    _RemoveKeys(plist, 'SCMPath')


def _DoPDFKeys(plist, add_keys):
  """Adds PDF support to the document types list. If add_keys is True, it will
  add the type information dictionary. If it is False, it will remove it if
  present."""

  PDF_FILE_EXTENSION = 'pdf'

  def __AddPDFKeys(sub_plist):
    """Writes the keys into a sub-dictionary of the plist."""
    sub_plist['CFBundleTypeExtensions'] = [PDF_FILE_EXTENSION]
    sub_plist['CFBundleTypeIconFile'] = 'document.icns'
    sub_plist['CFBundleTypeMIMETypes'] = 'application/pdf'
    sub_plist['CFBundleTypeName'] = 'PDF Document'
    sub_plist['CFBundleTypeRole'] = 'Viewer'

  DOCUMENT_TYPES_KEY = 'CFBundleDocumentTypes'

  
  try:
    extensions = plist[DOCUMENT_TYPES_KEY]
  except KeyError:
    
    
    if not add_keys:
      return
    extensions = plist[DOCUMENT_TYPES_KEY] = []

  
  for i, ext in enumerate(extensions):
    
    if 'CFBundleTypeExtensions' not in ext:
      continue
    if PDF_FILE_EXTENSION in ext['CFBundleTypeExtensions']:
      if add_keys:
        
        __AddPDFKeys(ext)
      else:
        
        del extensions[i]
      return

  
  if add_keys:
    pdf_entry = {}
    __AddPDFKeys(pdf_entry)
    extensions.append(pdf_entry)


def _AddBreakpadKeys(plist, branding):
  """Adds the Breakpad keys. This must be called AFTER _AddVersionKeys() and
  also requires the |branding| argument."""
  plist['BreakpadReportInterval'] = '3600'  
  plist['BreakpadProduct'] = '%s_Mac' % branding
  plist['BreakpadProductDisplay'] = branding
  plist['BreakpadVersion'] = plist['CFBundleShortVersionString']
  
  plist['BreakpadSendAndExit'] = 'YES'
  plist['BreakpadSkipConfirm'] = 'YES'


def _RemoveBreakpadKeys(plist):
  """Removes any set Breakpad keys."""
  _RemoveKeys(plist,
      'BreakpadURL',
      'BreakpadReportInterval',
      'BreakpadProduct',
      'BreakpadProductDisplay',
      'BreakpadVersion',
      'BreakpadSendAndExit',
      'BreakpadSkipConfirm')


def _AddKeystoneKeys(plist, bundle_identifier):
  """Adds the Keystone keys. This must be called AFTER _AddVersionKeys() and
  also requires the |bundle_identifier| argument (com.example.product)."""
  plist['KSVersion'] = plist['CFBundleShortVersionString']
  plist['KSProductID'] = bundle_identifier
  plist['KSUpdateURL'] = 'https://tools.google.com/service/update2'


def _RemoveKeystoneKeys(plist):
  """Removes any set Keystone keys."""
  _RemoveKeys(plist,
      'KSVersion',
      'KSProductID',
      'KSUpdateURL')


def Main(argv):
  parser = optparse.OptionParser('%prog [options]')
  parser.add_option('--breakpad', dest='use_breakpad', action='store',
      type='int', default=False, help='Enable Breakpad [1 or 0]')
  parser.add_option('--breakpad_uploads', dest='breakpad_uploads',
      action='store', type='int', default=False,
      help='Enable Breakpad\'s uploading of crash dumps [1 or 0]')
  parser.add_option('--keystone', dest='use_keystone', action='store',
      type='int', default=False, help='Enable Keystone [1 or 0]')
  parser.add_option('--scm', dest='add_scm_info', action='store', type='int',
      default=True, help='Add SCM metadata [1 or 0]')
  parser.add_option('--pdf', dest='add_pdf_support', action='store', type='int',
      default=False, help='Add PDF file handler support [1 or 0]')
  parser.add_option('--branding', dest='branding', action='store',
      type='string', default=None, help='The branding of the binary')
  parser.add_option('--bundle_id', dest='bundle_identifier',
      action='store', type='string', default=None,
      help='The bundle id of the binary')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >>sys.stderr, parser.get_usage()
    return 1

  
  DEST_INFO_PLIST = os.path.join(env['TARGET_BUILD_DIR'], env['INFOPLIST_PATH'])
  plist = plistlib.readPlist(DEST_INFO_PLIST)

  
  if not _AddVersionKeys(plist):
    return 2

  
  if options.use_breakpad:
    if options.branding is None:
      print >>sys.stderr, 'Use of Breakpad requires branding.'
      return 1
    _AddBreakpadKeys(plist, options.branding)
    if options.breakpad_uploads:
      plist['BreakpadURL'] = 'https://clients2.google.com/cr/report'
    else:
      
      
      
      
      
      
      plist['BreakpadURL'] = 'none'
  else:
    _RemoveBreakpadKeys(plist)

  
  if options.use_keystone and env['CONFIGURATION'] == 'Release':
    if options.bundle_identifier is None:
      print >>sys.stderr, 'Use of Keystone requires the bundle id.'
      return 1
    _AddKeystoneKeys(plist, options.bundle_identifier)
  else:
    _RemoveKeystoneKeys(plist)

  
  _DoSCMKeys(plist, options.add_scm_info)

  
  _DoPDFKeys(plist, options.add_pdf_support)

  
  temp_info_plist = tempfile.NamedTemporaryFile()
  plistlib.writePlist(plist, temp_info_plist.name)

  
  
  proc = subprocess.Popen(['plutil', '-convert', 'xml1', '-o', DEST_INFO_PLIST,
                           temp_info_plist.name])
  proc.wait()
  return proc.returncode


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
