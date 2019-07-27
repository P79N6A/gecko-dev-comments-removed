



import os
import zipfile
import simplejson as json
from cuddlefish.util import filter_filenames, filter_dirnames

class HarnessOptionAlreadyDefinedError(Exception):
    """You cannot use --harness-option on keys that already exist in
    harness-options.json"""

ZIPSEP = "/" 

def make_zipfile_path(localroot, localpath):
    return ZIPSEP.join(localpath[len(localroot)+1:].split(os.sep))

def mkzipdir(zf, path):
    dirinfo = zipfile.ZipInfo(path)
    dirinfo.external_attr = int("040755", 8) << 16L
    zf.writestr(dirinfo, "")

def build_xpi(template_root_dir, manifest, xpi_path,
              harness_options, limit_to=None, extra_harness_options={},
              bundle_sdk=True, pkgdir=""):
    IGNORED_FILES = [".hgignore", ".DS_Store",
                     "application.ini", xpi_path]
    IGNORED_TOP_LVL_FILES = ["install.rdf"]

    files_to_copy = {} 
    dirs_to_create = set() 

    zf = zipfile.ZipFile(xpi_path, "w", zipfile.ZIP_DEFLATED)

    zf.writestr('install.rdf', str(manifest))

    
    if 'icon' in harness_options:
        zf.write(os.path.join(str(harness_options['icon'])), 'icon.png')
        del harness_options['icon']

    if 'icon64' in harness_options:
        zf.write(os.path.join(str(harness_options['icon64'])), 'icon64.png')
        del harness_options['icon64']

    
    if os.path.isfile(os.path.join(pkgdir, 'chrome.manifest')):
      files_to_copy['chrome.manifest'] = os.path.join(pkgdir, 'chrome.manifest')

    
    folder = 'chrome'
    if os.path.exists(os.path.join(pkgdir, folder)):
      dirs_to_create.add('chrome')
      
      abs_dirname = os.path.join(pkgdir, folder)
      for dirpath, dirnames, filenames in os.walk(abs_dirname):
          goodfiles = list(filter_filenames(filenames, IGNORED_FILES))
          dirnames[:] = filter_dirnames(dirnames)
          for dirname in dirnames:
            arcpath = make_zipfile_path(template_root_dir,
                                        os.path.join(dirpath, dirname))
            dirs_to_create.add(arcpath)
          for filename in goodfiles:
              abspath = os.path.join(dirpath, filename)
              arcpath = ZIPSEP.join(
                  [folder,
                   make_zipfile_path(abs_dirname, os.path.join(dirpath, filename)),
                   ])
              files_to_copy[str(arcpath)] = str(abspath)

    for dirpath, dirnames, filenames in os.walk(template_root_dir):
        if template_root_dir == dirpath:
            filenames = list(filter_filenames(filenames, IGNORED_TOP_LVL_FILES))
        filenames = list(filter_filenames(filenames, IGNORED_FILES))
        dirnames[:] = filter_dirnames(dirnames)
        for dirname in dirnames:
            arcpath = make_zipfile_path(template_root_dir,
                                        os.path.join(dirpath, dirname))
            dirs_to_create.add(arcpath)
        for filename in filenames:
            abspath = os.path.join(dirpath, filename)
            arcpath = make_zipfile_path(template_root_dir, abspath)
            files_to_copy[arcpath] = abspath

    
    
    for packageName in harness_options['packages']:
      base_arcpath = ZIPSEP.join(['resources', packageName])
      
      
      if not bundle_sdk and packageName == 'addon-sdk':
          continue
      
      
      dirs_to_create.add(base_arcpath)
      for sectionName in harness_options['packages'][packageName]:
        abs_dirname = harness_options['packages'][packageName][sectionName]
        base_arcpath = ZIPSEP.join(['resources', packageName, sectionName])
        
        
        dirs_to_create.add(base_arcpath)
        
        for dirpath, dirnames, filenames in os.walk(abs_dirname):
            goodfiles = list(filter_filenames(filenames, IGNORED_FILES))
            dirnames[:] = filter_dirnames(dirnames)
            for filename in goodfiles:
                abspath = os.path.join(dirpath, filename)
                if limit_to is not None and abspath not in limit_to:
                    continue  
                arcpath = ZIPSEP.join(
                    ['resources',
                     packageName,
                     sectionName,
                     make_zipfile_path(abs_dirname,
                                       os.path.join(dirpath, filename)),
                     ])
                files_to_copy[str(arcpath)] = str(abspath)
    del harness_options['packages']

    locales_json_data = {"locales": []}
    mkzipdir(zf, "locale/")
    for language in sorted(harness_options['locale']):
        locales_json_data["locales"].append(language)
        locale = harness_options['locale'][language]
        
        jsonStr = json.dumps(locale, indent=1, sort_keys=True, ensure_ascii=False)
        info = zipfile.ZipInfo('locale/' + language + '.json')
        info.external_attr = 0644 << 16L
        zf.writestr(info, jsonStr.encode( "utf-8" ))
    del harness_options['locale']

    jsonStr = json.dumps(locales_json_data, ensure_ascii=True) +"\n"
    info = zipfile.ZipInfo('locales.json')
    info.external_attr = 0644 << 16L
    zf.writestr(info, jsonStr.encode("utf-8"))

    
    for arcpath in files_to_copy:
        bits = arcpath.split("/")
        for i in range(1,len(bits)):
            parentpath = ZIPSEP.join(bits[0:i])
            dirs_to_create.add(parentpath)

    
    
    for name in sorted(dirs_to_create.union(set(files_to_copy))):
        if name in dirs_to_create:
            mkzipdir(zf, name+"/")
        if name in files_to_copy:
            zf.write(files_to_copy[name], name)

    
    harness_options = harness_options.copy()
    for key,value in extra_harness_options.items():
        if key in harness_options:
            msg = "Can't use --harness-option for existing key '%s'" % key
            raise HarnessOptionAlreadyDefinedError(msg)
        harness_options[key] = value

    
    zf.writestr('harness-options.json', json.dumps(harness_options, indent=1,
                                                   sort_keys=True))

    zf.close()
