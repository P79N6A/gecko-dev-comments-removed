




































"""This routine controls which localizable files and entries are
reported and l10n-merged.
It's common to all of mobile, mobile/android and mobile/xul, so
those three versions need to stay in sync.
"""

def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "services/sync", "mobile",
                 "mobile/android/base",  "mobile/android",
                 "mobile/xul"):
    return "ignore"

  if mod not in ("mobile", "mobile/android", "mobile/xul"):
    
    return "error"
  if mod in ("mobile/android", "mobile/xul"):
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
