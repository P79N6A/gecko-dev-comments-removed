




































import string, sys

def main(argv):
    template = argv[1]
    unittests = argv[2:]

    includes = '\n'.join([
        '#include "%s.h"'% (t) for t in unittests ])


    enum_values = '\n'.join([
        '    %s,'% (t) for t in unittests ])
    last_enum = unittests[-1]


    string_to_enums = '\n'.join([
        '''    else if (!strcmp(aString, "%s"))
        return %s;'''% (t, t) for t in unittests ])

    enum_to_strings = '\n'.join([
        '''    case %s:
        return "%s";'''%(t, t) for t in unittests ])


    parent_main_cases = '\n'.join([
'''    case %s: {
        %sParent** parent =
        reinterpret_cast<%sParent**>(&gParentActor);
        *parent = new %sParent();
        (*parent)->Open(transport);
        return (*parent)->Main();
        }
'''% (t, t, t, t) for t in unittests ])


    child_init_cases = '\n'.join([
'''    case %s: {
        %sChild** child =
            reinterpret_cast<%sChild**>(&gChildActor);
        *child = new %sChild();
        (*child)->Open(transport, worker);
        return;
    }
'''% (t, t, t, t) for t in unittests ])

    child_cleanup_cases = '\n'.join([
'''    case %s: {
        %sChild** child =
            reinterpret_cast<%sChild**>(&gChildActor);
        delete *child;
        *child = 0;
        return;
    }
'''% (t, t, t) for t in unittests ])


    templatefile = open(template, 'r')
    sys.stdout.write(
        string.Template(templatefile.read()).substitute(
            INCLUDES=includes,
            ENUM_VALUES=enum_values, LAST_ENUM=last_enum,
            STRING_TO_ENUMS=string_to_enums,
            ENUM_TO_STRINGS=enum_to_strings,
            PARENT_MAIN_CASES=parent_main_cases,
            CHILD_INIT_CASES=child_init_cases,
            CHILD_CLEANUP_CASES=child_cleanup_cases))
    templatefile.close()

if __name__ == '__main__':
    main(sys.argv)
