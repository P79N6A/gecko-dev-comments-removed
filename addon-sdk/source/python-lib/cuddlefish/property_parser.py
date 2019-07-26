



import re
import codecs

class MalformedLocaleFileError(Exception):
    pass

def parse_file(path):
    return parse(read_file(path), path)

def read_file(path):
    try:
        return codecs.open( path, "r", "utf-8" ).readlines()
    except UnicodeDecodeError, e:
        raise MalformedLocaleFileError(
          'Following locale file is not a valid ' +
          'UTF-8 file: %s\n%s"' % (path, str(e)))

COMMENT = re.compile(r'\s*#')
EMPTY = re.compile(r'^\s+$')
KEYVALUE = re.compile(r"\s*([^=:]+)(=|:)\s*(.*)")

def parse(lines, path=None):
    lines = iter(lines)
    lineNo = 1
    pairs = dict()
    for line in lines:
        if COMMENT.match(line) or EMPTY.match(line) or len(line) == 0:
            continue
        m = KEYVALUE.match(line)
        if not m:
            raise MalformedLocaleFileError(
                  'Following locale file is not a valid .properties file: %s\n'
                  'Line %d is incorrect:\n%s' % (path, lineNo, line))

        
        
        key = m.group(1).rstrip()
        val = m.group(3).rstrip()
        val = val.encode('raw-unicode-escape').decode('raw-unicode-escape')

        
        if not key:
            raise MalformedLocaleFileError(
                  'Following locale file is not a valid .properties file: %s\n'
                  'Key is invalid on line %d is incorrect:\n%s' %
                  (path, lineNo, line))

        
        
        
        if val.endswith("\\"):
            val = val[:-1]
            try:
                
                line = lines.next().strip()
                while line.endswith("\\"):
                    val += line[:-1].lstrip()
                    line = lines.next()
                    lineNo += 1
                val += line.strip()
            except StopIteration:
                raise MalformedLocaleFileError(
                  'Following locale file is not a valid .properties file: %s\n'
                  'Unexpected EOF in multiline sequence at line %d:\n%s' %
                  (path, lineNo, line))
        
        pairs[key] = val
        lineNo += 1

    normalize_plural(path, pairs)
    return pairs











PLURAL_FORM = re.compile(r'^(.*)\[(zero|one|two|few|many|other)\]$')
def normalize_plural(path, pairs):
    for key in list(pairs.keys()):
        m = PLURAL_FORM.match(key)
        if not m:
            continue
        main_key = m.group(1)
        plural_form = m.group(2)
        
        if not main_key in pairs:
            pairs[main_key] = {}
            
            if not main_key + "[other]" in pairs:
                raise MalformedLocaleFileError(
                      'Following locale file is not a valid UTF-8 file: %s\n'
                      'This plural form doesn\'t have a matching `%s[other]` form:\n'
                      '%s\n'
                      'You have to defined following key:\n%s'
                      % (path, main_key, key, main_key))
        
        if isinstance(pairs[main_key], unicode):
            pairs[main_key] = {"other": pairs[main_key]}
        
        pairs[main_key][plural_form] = pairs[key]
        del pairs[key]
