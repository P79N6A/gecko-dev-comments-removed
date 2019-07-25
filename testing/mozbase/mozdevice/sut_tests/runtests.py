



from optparse import OptionParser
import os
import re
import unittest
import sys

import dmunit
import genfiles


def main(ip, port, scripts, directory, isTestDevice):
    dmunit.ip = ip
    dmunit.port = port

    suite = unittest.TestSuite()

    genfiles.gen_test_files()

    if scripts:
        
        
        scripts = map(lambda x: x.split('.')[0], scripts)
    else:
        
        
        testfile = re.compile('^test_.*\.py$')
        files = os.listdir(directory)

        for f in files:
            if testfile.match(f):
                scripts.append(f.split('.')[0])

    testLoader = dmunit.DeviceManagerTestLoader(isTestDevice)
    for s in scripts:
        suite.addTest(testLoader.loadTestsFromModuleName(s))
    unittest.TextTestRunner(verbosity=2).run(suite)

    genfiles.clean_test_files()


if  __name__ == "__main__":

    default_ip = '127.0.0.1'
    default_port = 20701

    env_ip, _, env_port = os.getenv('TEST_DEVICE', '').partition(':')
    if env_port:
        try:
            env_port = int(env_port)
        except ValueError:
            print >> sys.stderr, "Port in TEST_DEVICE should be an integer."
            sys.exit(1)

    
    parser = OptionParser()
    parser.add_option("--ip", action="store", type="string", dest="ip",
                      help="IP address for device running SUTAgent, defaults "
                      "to what's provided in $TEST_DEVICE or 127.0.0.1",
                      default=(env_ip or default_ip))

    parser.add_option("--port", action="store", type="int", dest="port",
                      help="Port of SUTAgent on device, defaults to "
                      "what's provided in $TEST_DEVICE or 20701",
                      default=(env_port or default_port))

    parser.add_option("--script", action="append", type="string",
                      dest="scripts", help="Name of test script to run, "
                      "can be specified multiple times", default=[])

    parser.add_option("--directory", action="store", type="string", dest="dir",
                      help="Directory to look for tests in, defaults to "
                      "current directory", default=os.getcwd())

    parser.add_option("--testDevice", action="store_true", dest="isTestDevice",
                      help="Specifies that the device is a local test agent",
                      default=False)

    (options, args) = parser.parse_args()

    main(options.ip, options.port, options.scripts,
         options.dir, options.isTestDevice)
