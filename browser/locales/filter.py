



def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "browser", "webapprt",
                 "extensions/reporter", "extensions/spellcheck",
                 "other-licenses/branding/firefox",
                 "browser/branding/official",
                 "services/sync"):
    return "ignore"
  if mod not in ("browser", "extensions/spellcheck"):
    
    return "error"
  if not entity:
    
    if mod == "extensions/spellcheck":
      return "ignore"
    
    if (re.match(r"searchplugins\/.+\.xml", path)):
      return "ignore"
    return "error"
  if mod == "extensions/spellcheck":
    
    return "error"
  if path == "defines.inc":
    return "ignore" if entity == "MOZ_LANGPACK_CONTRIBUTORS" else "error"

  if mod == "browser" and path == "chrome/browser-region/region.properties":
    
    return ("ignore"
            if (re.match(r"browser\.search\.order\.[1-9]", entity) or
                re.match(r"browser\.contentHandlers\.types\.[0-5]", entity) or
                re.match(r"gecko\.handlerService\.schemes\.", entity) or
                re.match(r"gecko\.handlerService\.defaultHandlersVersion", entity))
            else "error")
  return "error"
