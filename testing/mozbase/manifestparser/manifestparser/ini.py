



__all__ = ['read_ini']

import os

def read_ini(fp, variables=None, default='DEFAULT', defaults_only=False,
             comments=';#', separators=('=', ':'),
             strict=True):
    """
    read an .ini file and return a list of [(section, values)]
    - fp : file pointer or path to read
    - variables : default set of variables
    - default : name of the section for the default section
    - defaults_only : if True, return the default section only
    - comments : characters that if they start a line denote a comment
    - separators : strings that denote key, value separation in order
    - strict : whether to be strict about parsing
    """

    
    variables = variables or {}
    sections = []
    key = value = None
    section_names = set()
    if isinstance(fp, basestring):
        fp = file(fp)

    
    for (linenum, line) in enumerate(fp.readlines(), start=1):

        stripped = line.strip()

        
        if not stripped:
            
            key = value = None
            continue

        
        if stripped[0] in comments:
            continue

        
        if len(stripped) > 2 and stripped[0] == '[' and stripped[-1] == ']':
            section = stripped[1:-1].strip()
            key = value = None

            
            if section.lower() == default.lower():
                if strict:
                    assert default not in section_names
                section_names.add(default)
                current_section = variables
                continue

            if strict:
                
                assert section not in section_names, "Section '%s' already found in '%s'" % (section, section_names)

            section_names.add(section)
            current_section = {}
            sections.append((section, current_section))
            continue

        
        if not section_names:
            raise Exception('No sections found')

        
        for separator in separators:
            if separator in stripped:
                key, value = stripped.split(separator, 1)
                key = key.strip()
                value = value.strip()

                if strict:
                    
                    assert key
                    if current_section is not variables:
                        assert key not in current_section

                current_section[key] = value
                break
        else:
            
            if line[0].isspace() and key:
                value = '%s%s%s' % (value, os.linesep, stripped)
                current_section[key] = value
            else:
                
                if hasattr(fp, 'name'):
                    filename = fp.name
                else:
                    filename = 'unknown'
                raise Exception("Error parsing manifest file '%s', line %s" %
                                (filename, linenum))

    
    
    if 'server-root' in variables:
        root = os.path.join(os.path.dirname(fp.name),
                            variables['server-root'])
        variables['server-root'] = os.path.abspath(root)

    
    if defaults_only:
        return [(default, variables)]

    
    def interpret_variables(global_dict, local_dict):
        variables = global_dict.copy()
        if 'skip-if' in local_dict and 'skip-if' in variables:
            local_dict['skip-if'] = "(%s) || (%s)" % (variables['skip-if'].split('#')[0], local_dict['skip-if'].split('#')[0])
        variables.update(local_dict)

        return variables

    sections = [(i, interpret_variables(variables, j)) for i, j in sections]
    return sections



