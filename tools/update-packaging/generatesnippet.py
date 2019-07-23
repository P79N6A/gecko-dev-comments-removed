



































"""
This script generates the complete snippet for a given locale or en-US
Most of the parameters received are to generate the MAR's download URL
and determine the MAR's filename
"""
import sys, os, platform, sha
from optparse import OptionParser
from ConfigParser import ConfigParser
from stat import ST_SIZE

def main():
    error = False
    parser = OptionParser(
        usage="%prog [options]")
    parser.add_option("--mar-path",
                      action="store",
                      dest="marPath",
                      help="[Required] Specify the absolute path where the MAR file is found.")
    parser.add_option("--application-ini-file",
                      action="store",
                      dest="applicationIniFile",
                      help="[Required] Specify the absolute path to the application.ini file.")
    parser.add_option("-l",
                      "--locale",
                      action="store",
                      dest="locale",
                      help="[Required] Specify which locale we are generating the snippet for.")
    parser.add_option("-p",
                      "--product",
                      action="store",
                      dest="product",
                      help="[Required] This option is used to generate the URL to download the MAR file.")
    parser.add_option("--download-base-URL",
                      action="store",
                      dest="downloadBaseURL",
                      help="This option indicates under which.")
    parser.add_option("-v",
                      "--verbose",
                      action="store_true",
                      dest="verbose",
                      default=False,
                      help="This option increases the output of the script.")
    (options, args) = parser.parse_args()
    for req, msg in (('marPath', "the absolute path to the where the MAR file is"),
                     ('applicationIniFile', "the absolute path to the application.ini file."),
                     ('locale', "a locale."),
                     ('product', "specify a product.")):
        if not hasattr(options, req):
            parser.error('You must specify %s' % msg)

    if not options.downloadBaseURL or options.downloadBaseURL == '':
        options.downloadBaseURL = 'http://ftp.mozilla.org/pub/mozilla.org/%s/nightly' % options.product

    snippet = generateSnippet(options.marPath,
                              options.applicationIniFile,
                              options.locale,
                              options.downloadBaseURL,
                              options.product)
    f = open(os.path.join(options.marPath, 'complete.update.snippet'), 'wb')
    f.write(snippet)
    f.close()

    if options.verbose:
        
        print snippet

def generateSnippet(abstDistDir, applicationIniFile, locale,
                    downloadBaseURL, product):
    
    c = ConfigParser()
    try:
        c.readfp(open(applicationIniFile))
    except IOError, (stderror):
       sys.exit(stderror) 
    buildid = c.get("App", "BuildID")
    appVersion = c.get("App", "Version")
    branchName = c.get("App", "SourceRepository").split('/')[-1]

    marFileName = '%s-%s.%s.%s.complete.mar' % (
        product,
        appVersion,
        locale,
        getPlatform())
    
    
    (completeMarHash, completeMarSize) = getFileHashAndSize(
        os.path.join(abstDistDir, marFileName))
    
    interfix = ''
    if locale == 'en-US':
        interfix = ''
    else:
        interfix = '-l10n'
    marDownloadURL = "%s/%s%s/%s" % (downloadBaseURL,
                                     datedDirPath(buildid, branchName),
                                     interfix,
                                     marFileName)
    
    snippet = """complete
%(marDownloadURL)s
sha1
%(completeMarHash)s
%(completeMarSize)s
%(buildid)s
%(appVersion)s
%(appVersion)s
""" % dict( marDownloadURL=marDownloadURL, 
            completeMarHash=completeMarHash,
            completeMarSize=completeMarSize,
            buildid=buildid,
            appVersion=appVersion)

    return snippet

def getPlatform():
    if platform.system() == "Linux":
        return "linux-i686"
    elif platform.system() in ("Windows", "Microsoft"):
        return "win32"
    elif platform.system() == "Darwin":
        return "mac"

def getFileHashAndSize(filepath):
    sha1Hash = 'UNKNOWN'
    size = 'UNKNOWN'

    try:
        
        
        f = open(filepath, "rb")
        shaObj = sha.new(f.read())
        sha1Hash = shaObj.hexdigest()
        size = os.stat(filepath)[ST_SIZE]
    except IOError, (stderror):
       sys.exit(stderror) 

    return (sha1Hash, size)

def datedDirPath(buildid, milestone):
    """
    Returns a string that will look like:
    2009/12/2009-12-31-09-mozilla-central
    """
    year  = buildid[0:4]
    month = buildid[4:6]
    day   = buildid[6:8]
    hour  = buildid[8:10]
    datedDir = "%s-%s-%s-%s-%s" % (year,
                                   month,
                                   day,
                                   hour,
                                   milestone)
    return "%s/%s/%s" % (year, month, datedDir)

if __name__ == '__main__':
    main()
