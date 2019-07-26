



def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "browser", "browser/metro", "webapprt",
                 "extensions/reporter", "extensions/spellcheck",
                 "other-licenses/branding/firefox",
                 "browser/branding/official",
                 "services/sync"):
    return "ignore"
  if mod not in ("browser", "browser/metro", "extensions/spellcheck"):
    
    return "error"
  if not entity:
    
    if mod == "extensions/spellcheck":
      return "ignore"
    
    return "ignore" if re.match(r"searchplugins\/.+\.xml", path) else "error"
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
  if mod == "browser/metro" and path == "chrome/region.properties":
      return ("ignore"
              if re.match(r"browser\.search\.order\.[1-9]", entity)
              else "error")
  return "error"
