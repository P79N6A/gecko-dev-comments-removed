




































'Mozilla l10n compare locales tool'

import os
import re
import logging
import Parser
import Paths

def __regify(tpl):
  return tuple(map(re.compile, tpl))

exceptions = [
  
  __regify(('mail|browser', 'defines.inc', 'MOZ_LANGPACK_CONTRIBUTORS')),
  
  __regify(('browser', 'chrome\\/browser-region\\/region\\.properties',
   'browser\\.search\\.order\.[1-9]')),
  
  __regify(('browser', 'chrome\\/browser-region\\/region\\.properties',
   'browser\\.contentHandlers\\.types\.[0-5]'))]

def __dont_ignore(tpl):
  for mod, path, key in exceptions:
    if mod.match(tpl[0]) and path.match(tpl[1]) and key.match(tpl[2]):
      return False
  return True

fl_exceptions = [
  
  __regify(('browser', 'searchplugins\\/.+\\.xml')),
  
  __regify(('browser', 'chrome\\/help\\/images\\/[A-Za-z-_]+\\.png'))]

def do_ignore_fl(tpl):
  for mod, path in fl_exceptions:
    if mod.match(tpl[0]) and path.match(tpl[1]):
      return True
  return False

class FileCollector:
  class Iter:
    def __init__(self, path):
      self.__base = path
    def __iter__(self):
      self.__w = os.walk(self.__base)
      try:
        self.__nextDir()
      except StopIteration:
        
        self.__i = [].__iter__()
      return self
    def __nextDir(self):
      self.__t = self.__w.next()
      try:
        self.__t[1].remove("CVS")
      except ValueError:
        pass
      self.__t[1].sort()
      self.__t[2].sort()
      self.__i = self.__t[2].__iter__()
    def next(self):
      try:
        leaf = self.__i.next()
        path = self.__t[0] + '/' + leaf
        key = path[len(self.__base) + 1:]
        return (key, path)
      except StopIteration:
        self.__nextDir()
        return self.next()
      print "not expected"
      raise StopIteration
  def __init__(self):
    pass
  def getFiles(self, mod, locale):
    fls = {}
    for leaf, path in self.iterateFiles(mod,locale):
      fls[leaf] = path
    return fls
  def iterateFiles(self, mod, locale):
    return FileCollector.Iter(Paths.get_base_path(mod, locale))

def collectFiles(aComparer, apps = None, locales = None):
  '''
  returns new files, files to compare, files to remove
  apps or locales need to be given, apps is a list, locales is a
  hash mapping applications to languages.
  If apps is given, it will look up all-locales for all apps for the
  languages to test.
  'toolkit' is added to the list of modules, too.
  '''
  if not apps and not locales:
    raise RuntimeError, "collectFiles needs either apps or locales"
  if apps and locales:
    raise RuntimeError, "You don't want to give both apps or locales"
  if locales:
    apps = locales.keys()
    
    all = set()
    for locs in locales.values():
      all.update(locs)
    locales['toolkit'] = list(all)
  else:
    locales = Paths.allLocales(apps)
  modules = Paths.Modules(apps)
  en = FileCollector()
  l10n = FileCollector()
  for cat in modules.keys():
    logging.debug(" testing " + cat+ " on " + str(modules))
    aComparer.notifyLocales(cat, locales[cat])
    for mod in modules[cat]:
      en_fls = en.getFiles(mod, 'en-US')
      for loc in locales[cat]:
        fls = dict(en_fls) 
        for l_fl, l_path in l10n.iterateFiles(mod, loc):
          if fls.has_key(l_fl):
            
            aComparer.compareFile(mod, loc, l_fl)
            del fls[l_fl]
          else:
            
            aComparer.removeFile(mod, loc, l_fl)
        
        for lf in fls.keys():
          aComparer.addFile(mod,loc,lf)

class CompareCollector:
  'collects files to be compared, added, removed'
  def __init__(self):
    self.cl = {}
    self.files = {}
    self.modules = {}
  def notifyLocales(self, aModule, aLocaleList):
    for loc in aLocaleList:
      if self.modules.has_key(loc):
        self.modules[loc].append(aModule)
      else:
        self.modules[loc] = [aModule]
  def addFile(self, aModule, aLocale, aLeaf):
    if do_ignore_fl((aModule, aLeaf)):
      logging.debug(" ignoring %s from %s in %s" % (aLeaf, aLocale, aModule))
      return
    logging.debug(" add %s for %s in %s" % (aLeaf, aLocale, aModule))
    if not self.files.has_key(aLocale):
      self.files[aLocale] = {'missingFiles': [(aModule, aLeaf)],
                             'obsoleteFiles': []}
    else:
      self.files[aLocale]['missingFiles'].append((aModule, aLeaf))
    pass
  def compareFile(self, aModule, aLocale, aLeaf):
    if not self.cl.has_key((aModule, aLeaf)):
      self.cl[(aModule, aLeaf)] = [aLocale]
    else:
      self.cl[(aModule, aLeaf)].append(aLocale)
    pass
  def removeFile(self, aModule, aLocale, aLeaf):
    if do_ignore_fl((aModule, aLeaf)):
      logging.debug(" ignoring %s from %s in %s" % (aLeaf, aLocale, aModule))
      return
    logging.debug(" remove %s from %s in %s" % (aLeaf, aLocale, aModule))
    if not self.files.has_key(aLocale):
      self.files[aLocale] = {'obsoleteFiles': [(aModule, aLeaf)],
                             'missingFiles':[]}
    else:
      self.files[aLocale]['obsoleteFiles'].append((aModule, aLeaf))
    pass

def compare(apps=None, testLocales=None):
  result = {}
  c = CompareCollector()
  collectFiles(c, apps=apps, locales=testLocales)
  key = re.compile('[kK]ey')
  for fl, locales in c.cl.iteritems():
    (mod,path) = fl
    try:
      parser = Parser.getParser(path)
    except UserWarning:
      logging.warning(" Can't compare " + path + " in " + mod)
      continue
    parser.read(Paths.get_path(mod, 'en-US', path))
    enMap = parser.mapping()
    for loc in locales:
      if not result.has_key(loc):
        result[loc] = {'missing':[],'obsolete':[],
                       'changed':0,'unchanged':0,'keys':0}
      enTmp = dict(enMap)
      parser.read(Paths.get_path(mod, loc, path))
      for k,v in parser:
        if not __dont_ignore((mod, path, k)):
          if enTmp.has_key(k):
            del enTmp[k]
          continue
        if not enTmp.has_key(k):
          result[loc]['obsolete'].append((mod,path,k))
          continue
        enVal = enTmp[k]
        del enTmp[k]
        if key.search(k):
          result[loc]['keys'] += 1
        else:
          if enVal == v:
            result[loc]['unchanged'] +=1
            logging.info('%s in %s unchanged' %
                         (k, Paths.get_path(mod, loc, path)))
          else:
            result[loc]['changed'] +=1
      result[loc]['missing'].extend(filter(__dont_ignore, [(mod,path,k) for k in enTmp.keys()]))
  for loc,dics in c.files.iteritems():
    if not result.has_key(loc):
      result[loc] = dics
    else:
      for key, list in dics.iteritems():
        result[loc][key] = list
  for loc, mods in c.modules.iteritems():
    result[loc]['tested'] = mods
  return result
