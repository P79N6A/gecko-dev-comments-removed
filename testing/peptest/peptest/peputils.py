



































import urllib2
import urlparse
import os
import zipfile
import tarfile

def download(url, savepath=''):
    """
    Save the file located at 'url' into 'savepath'
    If savepath is None, use the last part of the url path.
    Returns the path of the saved file.
    """
    data = urllib2.urlopen(url)
    if savepath == '' or os.path.isdir(savepath):
        parsed = urlparse.urlsplit(url)
        filename = parsed.path[parsed.path.rfind('/')+1:]
        savepath = os.path.join(savepath, filename)
    savedir = os.path.dirname(savepath)
    if savedir != '' and not os.path.exists(savedir):
        os.makedirs(savedir)
    outfile = open(savepath, 'wb')
    outfile.write(data.read())
    outfile.close()
    return os.path.realpath(savepath)

def isURL(path):
    """Return True if path looks like a URL."""
    if path is not None:
        return urlparse.urlparse(path).scheme != ''
    return False

def extract(path, extdir=None, delete=False):
    """
    Takes in a tar or zip file and extracts it to extdir
    If extdir is not specified, extracts to path
    If delete is set to True, deletes the bundle at path
    Returns the list of top level files that were extracted
    """
    if zipfile.is_zipfile(path):
        bundle = zipfile.ZipFile(path)
        namelist = bundle.namelist()
    elif tarfile.is_tarfile(path):
        bundle = tarfile.open(path)
        namelist = bundle.getnames()
    else:
        return
    if extdir is None:
        extdir = os.path.dirname(path)
    elif not os.path.exists(extdir):
        os.makedirs(extdir)
    bundle.extractall(path=extdir)
    bundle.close()
    if delete:
        os.remove(path)
    return [os.path.join(extdir, name) for name in namelist
                if len(name.rstrip(os.sep).split(os.sep)) == 1]
