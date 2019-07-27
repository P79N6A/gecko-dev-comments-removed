
# encoding: utf-8

"""count_and_reboot.py [-n maxcount] -f countfile

Increments the value in countfile, and reboots the machine once the count
reaches or exceeds maxcount."""

import os, sys, time

if sys.platform in ('darwin', 'linux2'):
    def reboot():
        
        
        
        
        os.system("sudo -S reboot < /dev/null")
	
	
	time.sleep(600)

elif sys.platform == "win32":
    
    def reboot():
        os.system("shutdown -f -r -t 0")
	
	
	time.sleep(600)

def increment_count(fname):
    try:
        current_count = int(open(fname).read())
    except:
        current_count = 0
    current_count += 1
    open(fname, "w").write("%i\n" % current_count)
    return current_count

if __name__ == '__main__':
    from optparse import OptionParser

    parser = OptionParser(__doc__)
    parser.add_option("-n", "--max-count", dest="maxcount", default=10,
            help="reboot after <maxcount> runs", type="int")
    parser.add_option("-f", "--count-file", dest="countfile", default=None,
            help="file to record count in")
    parser.add_option("-z", "--zero-count", dest="zero", default=False,
            action="store_true", help="zero out the counter before rebooting")

    options, args = parser.parse_args()

    if not options.countfile:
        parser.error("countfile is required")

    if increment_count(options.countfile) >= options.maxcount:
        if options.zero:
            open(options.countfile, "w").write("0\n")
        print "************************************************************************************************"
        print "*********** END OF RUN - NOW DOING SCHEDULED REBOOT; FOLLOWING ERROR MESSAGE EXPECTED **********"
        print "************************************************************************************************"
        sys.stdout.flush()
        reboot()
