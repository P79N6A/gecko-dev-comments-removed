



"""This routine controls which localizable files and entries are
reported and l10n-merged.
This needs to stay in sync with the copy in mobile/android/locales.
"""

def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "services/sync", "mobile",
                 "mobile/android/base",  "mobile/android"):
    return "ignore"

  if mod not in ("mobile", "mobile/android"):
    
    return "error"
  if mod == "mobile/android":
    if not entity:
      if (re.match(r"mobile-l10n.js", path) or
          re.match(r"defines.inc", path)):
        return "ignore"
    if path == "defines.inc":
      if entity == "MOZ_LANGPACK_CONTRIBUTORS":
        return "ignore"
    return "error"

  
  if re.match(r"searchplugins\/.+\.xml", path):
    return "ignore"
  if path == "chrome/region.properties":
    
    if (re.match(r"browser\.search\.order\.[1-9]", entity) or
        re.match(r"browser\.contentHandlers\.types\.[0-5]", entity) or
        re.match(r"gecko\.handlerService\.schemes\.", entity) or
      re.match(r"gecko\.handlerService\.defaultHandlersVersion", entity)):
      return "ignore"

  return "error"
