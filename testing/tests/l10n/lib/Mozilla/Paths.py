




































modules = {'toolkit': ['netwerk','dom','toolkit','security/manager'],
           'browser': ['browser','extensions/reporter',
                       'other-licenses/branding/firefox'],
           'mail': ['mail','other-licenses/branding/thunderbird',
                    'editor/ui']}

components = {}
for mod, lst in modules.iteritems():
  for c in lst:
    components[c] = mod

locales = {}
all = {}
for app in ['browser', 'mail']:
  path = 'mozilla/%s/locales/all-locales' % app
  locales[app] = [l.strip() for l in open(path)]
  for loc in locales[app]: all[loc] = 1
all = sorted(all.keys())
locales['toolkit'] = all


pass

def get_base_path(mod, loc):
  'statics for path patterns and conversion'
  __l10n = 'l10n/%(loc)s/%(mod)s'
  __en_US = 'mozilla/%(mod)s/locales/en-US'
  if loc == 'en-US':
    return __en_US % {'mod': mod}
  return __l10n % {'mod': mod, 'loc': loc}

def get_path(mod, loc, leaf):
  return get_base_path(mod, loc) + '/' + leaf

