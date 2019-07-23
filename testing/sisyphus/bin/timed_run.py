





































import os, signal, sys, time




exitOSError   = 66
exitSignal    = 77
exitTimeout   = 88
exitInterrupt = 99

pid = None
prefix = sys.argv[2]
elapsedtime = 0

if prefix == "-":
    prefix = ''
else:
    prefix = prefix + ':'

def alarm_handler(signum, frame):
    global pid
    global prefix
    try:
        print "%s EXIT STATUS: TIMED OUT (%s seconds)" % (prefix, sys.argv[1])
        os.kill(pid, signal.SIGKILL)
    except:
        pass
    sys.exit(exitTimeout)

def forkexec(command, args):
    global prefix
    global elapsedtime
    
    
    try:
        pid = os.fork()
        if pid == 0:  
            os.execvp(command, args)
        else:  
            return pid
    except OSError, e:
        print "%s ERROR: %s %s failed: %d (%s) (%f seconds)" % (prefix, command, args, e.errno, e.strerror, elapsedtime)
        sys.exit(exitOSError)

signal.signal(signal.SIGALRM, alarm_handler)
signal.alarm(int(sys.argv[1]))
starttime = time.time()
try:
	pid = forkexec(sys.argv[3], sys.argv[3:])
	status = os.waitpid(pid, 0)[1]
	signal.alarm(0) 
	stoptime = time.time()
	elapsedtime = stoptime - starttime
	
	
	if os.WIFSIGNALED(status):
	    print "%s EXIT STATUS: CRASHED signal %d (%f seconds)" % (prefix, os.WTERMSIG(status), elapsedtime)
	    sys.exit(exitSignal)
	elif os.WIFEXITED(status):
	    rc = os.WEXITSTATUS(status)
	    msg = ''
	    if rc == 0:
	        msg = 'NORMAL'
	    elif rc < 3:
	        msg = 'ABNORMAL ' + str(rc)
		rc = exitSignal
	    else:
	        msg = 'CRASHED ' + str(rc)
		rc = exitSignal

	    print "%s EXIT STATUS: %s (%f seconds)" % (prefix, msg, elapsedtime)
	    sys.exit(rc)
	else:
	    print "%s EXIT STATUS: NONE (%f seconds)" % (prefix, elapsedtime)
	    sys.exit(0)
except KeyboardInterrupt:
	os.kill(pid, 9)
	sys.exit(exitInterrupt)
