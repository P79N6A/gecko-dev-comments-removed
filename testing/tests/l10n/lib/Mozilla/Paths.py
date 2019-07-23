




































import os.path
from subprocess import *


class Modules(dict):
  '''
  Subclass of dict to hold information on which directories belong to a
  particular app.
  It expects to have mozilla/client.mk right there from the working dir,
  and asks that for the LOCALES_foo variables.
  This only works for toolkit applications, as it's assuming that the
  apps include toolkit.
  '''
  def __init__(self, apps):
    super(dict, self).__init__()
    lapps = apps[:]
    lapps.insert(0, 'toolkit')
    of  = os.popen('make -f mozilla/client.mk ' + \
                   ' '.join(['echo-variable-LOCALES_' + app for app in lapps]))
    
    for val in of.readlines():
      self[lapps.pop(0)] = val.strip().split()
    for k,v in self.iteritems():
      if k == 'toolkit':
        continue
      self[k] = [d for d in v if d not in self['toolkit']]

class Components(dict):
  '''
  Subclass of dict to map module dirs to applications. This reverses the
  mapping you'd get from a Modules class, and it in fact uses one to do
  its job.
  '''
  def __init__(self, apps):
    modules = Modules(apps)
    for mod, lst in modules.iteritems():
      for c in lst:
        self[c] = mod

def allLocales(apps):
  '''
  Get a locales hash for the given list of applications, mapping
  applications to the list of languages given by all-locales.
  Adds a module 'toolkit' holding all languages for all applications, too.
  '''
  locales = {}
  all = set()
  for app in apps:
    path = 'mozilla/%s/locales/all-locales' % app
    locales[app] = [l.strip() for l in open(path)]
    all.update(locales[app])
  locales['toolkit'] = list(all)
  return locales

def get_base_path(mod, loc):
  'statics for path patterns and conversion'
  __l10n = 'l10n/%(loc)s/%(mod)s'
  __en_US = 'mozilla/%(mod)s/locales/en-US'
  if loc == 'en-US':
    return __en_US % {'mod': mod}
  return __l10n % {'mod': mod, 'loc': loc}

def get_path(mod, loc, leaf):
  return get_base_path(mod, loc) + '/' + leaf

