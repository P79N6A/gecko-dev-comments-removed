def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "browser", "extensions/reporter", "extensions/spellcheck",
                 "other-licenses/branding/firefox",
                 "browser/branding/official",
                 "services/sync"):
    return False
  if mod != "browser" and mod != "extensions/spellcheck":
    
    return True
  if not entity:
    if mod == "extensions/spellcheck":
      return False
    
    return not (re.match(r"searchplugins\/.+\.xml", path) or
                re.match(r"chrome\/help\/images\/[A-Za-z-_]+\.png", path))
  if mod == "extensions/spellcheck":
    
    return True
  if path == "defines.inc":
    return entity != "MOZ_LANGPACK_CONTRIBUTORS"

  if path != "chrome/browser-region/region.properties":
    
    return True
  
  return not (re.match(r"browser\.search\.order\.[1-9]", entity) or
              re.match(r"browser\.contentHandlers\.types\.[0-5]", entity) or
              re.match(r"gecko\.handlerService\.schemes\.", entity) or
              re.match(r"gecko\.handlerService\.defaultHandlersVersion", entity))
