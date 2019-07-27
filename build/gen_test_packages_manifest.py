





import json

from argparse import ArgumentParser

ALL_HARNESSES = [
    'common', 
    'mochitest',
    'reftest',
    'webapprt',
    'xpcshell',
    'cppunittest',
    'jittest',
    'mozbase',
]

PACKAGE_SPECIFIED_HARNESSES = [
]


def parse_args():
    parser = ArgumentParser(description='Generate a test_packages.json file to tell automation which harnesses require which test packages.')
    parser.add_argument("--common", required=True,
                        action="store", dest="tests_common",
                        help="Name of the \"common\" archive, a package to be used by all harnesses.")
    parser.add_argument("--jsshell", required=True,
                        action="store", dest="jsshell",
                        help="Name of the jsshell zip.")
    for harness in PACKAGE_SPECIFIED_HARNESSES:
        parser.add_argument("--%s" % harness, required=True,
                            action="store", dest=harness,
                            help="Name of the %s zip." % harness)
    parser.add_argument("--dest-file", required=True,
                        action="store", dest="destfile",
                        help="Path to the output file to be written.")
    return parser.parse_args()

def generate_package_data(args):
    
    
    
    
    
    
    tests_common = args.tests_common
    jsshell = args.jsshell

    harness_requirements = dict([(k, [tests_common]) for k in ALL_HARNESSES])
    harness_requirements['jittest'].append(jsshell)
    for harness in PACKAGE_SPECIFIED_HARNESSES:
        harness_requirements[harness].append(getattr(args, harness))
    return harness_requirements

if __name__ == '__main__':
    args = parse_args()
    packages_data = generate_package_data(args)
    with open(args.destfile, 'w') as of:
        json.dump(packages_data, of, indent=4)
