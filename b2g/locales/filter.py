




def test(mod, path, entity = None):
  import re
  
  if mod not in ("netwerk", "dom", "toolkit", "security/manager",
                 "mobile",
                 "b2g"):
    return "ignore"

  return "error"
