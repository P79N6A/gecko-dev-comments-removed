











from optparse import OptionParser
import json
import re
import urllib2
import urlparse
import sys
import os

def main():
    '''
    Determine which repository and revision mozharness.json indicates.
    If none is found we fall back to the default repository
    '''
    parser = OptionParser()
    parser.add_option("--repository-manifest-url", dest="repository_manifest_url", type="string",
                      help="It indicates from where to download the talos.json file.")
    (options, args) = parser.parse_args()

    
    if options.repository_manifest_url == None:
        print "You need to specify --repository-manifest-url."
        sys.exit(1)

    
    try:
        jsonFilename = download_file(options.talos_json_url)
    except Exception, e:
        print "ERROR: We tried to download the talos.json file but something failed."
        print "ERROR: %s" % str(e)
        sys.exit(1)

    
    print "INFO: talos.json URL: %s" % options.talos_json_url
    try:
        key = 'talos.zip'
        entity = get_value(jsonFilename, key)
        if passesRestrictions(options.talos_json_url, entity["url"]):
            
            print "INFO: Downloading %s as %s" % (entity["url"], os.path.join(entity["path"], key))
            download_file(entity["url"], entity["path"], key)
        else:
            print "ERROR: You have tried to download a file " + \
                  "from: %s " % entity["url"] + \
                  "which is a location different than http://talos-bundles.pvt.build.mozilla.org/"
            print "ERROR: This is only allowed for the certain branches."
            sys.exit(1)
    except Exception, e:
        print "ERROR: %s" % str(e)
        sys.exit(1)

def passesRestrictions(talosJsonUrl, fileUrl):
    '''
    Only certain branches are exempted from having to host their downloadable files
    in talos-bundles.pvt.build.mozilla.org
    '''
    if talosJsonUrl.startswith("http://hg.mozilla.org/try/") or \
       talosJsonUrl.startswith("https://hg.mozilla.org/try/") or \
       talosJsonUrl.startswith("http://hg.mozilla.org/projects/pine/") or \
       talosJsonUrl.startswith("https://hg.mozilla.org/projects/pine/") or \
       talosJsonUrl.startswith("http://hg.mozilla.org/projects/ash/") or \
       talosJsonUrl.startswith("https://hg.mozilla.org/projects/ash/"):
        return True
    else:
        p = re.compile('^http://talos-bundles.pvt.build.mozilla.org/')
        m = p.match(fileUrl)
        if m == None:
            return False
        return True

def get_filename_from_url(url):
    '''
    This returns the filename of the file we're trying to download
    '''
    parsed = urlparse.urlsplit(url.rstrip('/'))
    if parsed.path != '':
        return parsed.path.rsplit('/', 1)[-1]
    else:
        print "ERROR: We were trying to download a file from %s " + \
              "but the URL seems to be incorrect."
        sys.exit(1)

def download_file(url, path="", saveAs=None):
    '''
    It downloads a file from URL to the indicated path
    '''
    req = urllib2.Request(url)
    f = urllib2.urlopen(req)
    if path != "" and not os.path.isdir(path):
        try:
            os.makedirs(path)
            print "INFO: directory %s created" % path
        except Exception, e:
            print "ERROR: %s" % str(e)
            sys.exit(1)
    filename = saveAs if saveAs else get_filename_from_url(url)
    local_file = open(os.path.join(path, filename), 'wb')
    local_file.write(f.read())
    local_file.close()
    return filename

def get_value(json_filename, key):
    '''
    It loads up a JSON file and returns the value for the given string
    '''
    f = open(json_filename, 'r')
    return json.load(f)[key]

if __name__ == '__main__':
    main()
