



































import devicemanager
import devicemanagerUtils
import sys
import os

def main():
    ip_addr = os.environ.get("DEVICE_IP")
    port = os.environ.get("DEVICE_PORT")
    gre_path = os.environ.get("REMOTE_GRE_PATH").replace('\\','/')

    if ip_addr == None:
        print "Error: please define the environment variable DEVICE_IP before running this test"
        sys.exit(1)

    if port == None:
        print "Error: please define the environment variable DEVICE_PORT before running this test"
        sys.exit(1)

    if gre_path == None:
        print "Error: please define the environment variable REMOTE_GRE_PATH before running this test"
        sys.exit(1)

    dm = devicemanagerUtils.getDeviceManager(ip_addr, int(port))
    if len(sys.argv) < 2:
        print "usage python devicemanager-run-test.py <test program> [args1 [arg2..]]"
        sys.exit(1)

    cmd = sys.argv[1]
    args = ' '
    if len(sys.argv) > 2:
        args = ' ' + ' '.join(sys.argv[2:])

    dm.debug = 0
    lastslash = cmd.rfind('/');
    if lastslash == -1:
        lastslash = 0
    dm.pushFile(cmd, gre_path + cmd[lastslash:])
    process = dm.launchProcess([gre_path  + cmd[lastslash:] + args])
    output_list = dm.communicate(process)
    ret = -1
    if (output_list != None and output_list[0] != None):
        try:
            output = output_list[0]
            index = output.find('exited with return code')
            print output[0:index]
            retstr = (output[index + 24 : output.find('\n', index)])
            ret = int(retstr)
        except ValueError:
            ret = -1
    sys.exit(ret)

if __name__ == '__main__':
    main()
