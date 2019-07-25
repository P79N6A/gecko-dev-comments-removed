







































from __future__ import with_statement

import sys
import os
import re
import bisect

def prettyFileName(name):
  if name.startswith("../") or name.startswith("..\\"):
    
    
    return os.path.basename(name) + ":"
  elif name.startswith("hg:"):
    (junk, repo, path, rev) = name.split(":")
    
    
    return path  + ":"
  return name  + ":"

class readSymbolFile:
  def __init__(self, fn):
    addrs = [] 
    funcs = {} 
    files = {} 
    with open(fn) as f:
      for line in f:
        line = line.rstrip()
        
        if line.startswith("FUNC "):
          
          (junk, rva, size, ss, name) = line.split(None, 4)
          rva = int(rva,16)
          funcs[rva] = name
          addrs.append(rva)
          lastFuncName = name
        elif line.startswith("PUBLIC "):
          
          (junk, rva, ss, name) = line.split(None, 3)
          rva = int(rva,16)
          funcs[rva] = name
          addrs.append(rva)
        elif line.startswith("FILE "):
          
          (junk, filenum, name) = line.split(None, 2)
          files[filenum] = prettyFileName(name)
        elif line[0] in "0123456789abcdef":
          
          
          (rva, size, line, filenum) = line.split(None)
          rva = int(rva,16)
          file = files[filenum]
          name = lastFuncName + " [" + file + line + "]"
          funcs[rva] = name
          addrs.append(rva)
        
    
    self.addrs = sorted(addrs)
    self.funcs = funcs
  def addrToSymbol(self, address):
    i = bisect.bisect(self.addrs, address) - 1
    if i > 0:
      
      return self.funcs[self.addrs[i]]
    else:
      return ""


def guessSymbolFile(fn, symbolsDir):
  """Guess a symbol file based on an object file's basename, ignoring the path and UUID."""
  fn = os.path.basename(fn)
  d1 = os.path.join(symbolsDir, fn)
  if not os.path.exists(d1):
    return None
  uuids = os.listdir(d1)
  if len(uuids) == 0:
    raise Exception("Missing symbol file for " + fn)
  if len(uuids) > 1:
    raise Exception("Ambiguous symbol file for " + fn)
  return os.path.join(d1, uuids[0], fn + ".sym")

parsedSymbolFiles = {}
def addressToSymbol(file, address, symbolsDir):
  p = None
  if not file in parsedSymbolFiles:
    symfile = guessSymbolFile(file, symbolsDir)
    if symfile:
      p = readSymbolFile(symfile)
    else:
      p = None
    parsedSymbolFiles[file] = p
  else:
    p = parsedSymbolFiles[file]
  if p:
    return p.addrToSymbol(address)
  else:
    return ""

line_re = re.compile("^(.*) ?\[([^ ]*) \+(0x[0-9A-F]{1,8})\](.*)$")
balance_tree_re = re.compile("^([ \|0-9-]*)")

def fixSymbols(line, symbolsDir):
  result = line_re.match(line)
  if result is not None:
    
    
    (before, file, address, after) = result.groups()
    address = int(address, 16)
    
    before = balance_tree_re.match(before).groups()[0]
    symbol = addressToSymbol(file, address, symbolsDir)
    if not symbol:
      symbol = "%s + 0x%x" % (os.path.basename(file), address)
    return before + symbol + after + "\n"
  else:
      return line

if __name__ == "__main__":
  symbolsDir = sys.argv[1]
  for line in iter(sys.stdin.readline, ''):
    print fixSymbols(line, symbolsDir),
