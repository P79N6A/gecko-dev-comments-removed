






import sys
import optparse

def main(argv):

  parser = optparse.OptionParser()
  usage = 'usage: %s [options ...] format_string locale_list'
  parser.set_usage(usage.replace('%s', '%prog'))
  parser.add_option('-d', dest='dash_to_underscore', action="store_true",
                    default=False,
                    help='map "en-US" to "en" and "-" to "_" in locales')

  (options, arglist) = parser.parse_args(argv)

  if len(arglist) < 3:
    print 'ERROR: need string and list of locales'
    return 1

  str_template = arglist[1]
  locales = arglist[2:]

  results = []
  for locale in locales:
    
    
    
    if options.dash_to_underscore:
      if locale == 'en-US':
        locale = 'en'
      locale = locale.replace('-', '_')
    results.append(str_template.replace('ZZLOCALE', locale))

  
  
  print ' '.join(["'%s'" % x for x in results])

if __name__ == '__main__':
  sys.exit(main(sys.argv))
