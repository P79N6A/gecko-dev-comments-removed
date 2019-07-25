




































from __future__ import with_statement

with open('crtdll.obj', 'rb') as infile:
  data = infile.read()
  with open('crtdll_fixed.obj', 'wb') as outfile:
    data = data.replace('__imp__free', '__imp__frex')
    outfile.write(data)
