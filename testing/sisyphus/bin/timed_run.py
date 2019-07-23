






































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

def getSignalName(num):
    for p in dir(signal):
        if p.startswith("SIG") and not p.startswith("SIG_"):
            if getattr(signal, p) == num:
                return p
    return "UNKNOWN"

def alarm_handler(signum, frame):
    global pid
    global prefix
    try:
	stoptime = time.time()
	elapsedtime = stoptime - starttime
        print "\n%s EXIT STATUS: TIMED OUT (%s seconds)\n" % (prefix, elapsedtime)
        flushkill(pid, signal.SIGKILL)
    except OSError, e:
        print "\ntimed_run.py: exception trying to kill process: %d (%s)\n" % (e.errno, e.strerror)
        pass
    flushexit(exitTimeout)

def forkexec(command, args):
    global prefix
    global elapsedtime
    
    
    try:
        pid = os.fork()
        if pid == 0:  
            os.execvp(command, args)
            flushbuffers()
        else:  
            return pid
    except OSError, e:
        print "\n%s ERROR: %s %s failed: %d (%s) (%f seconds)\n" % (prefix, command, args, e.errno, e.strerror, elapsedtime)
        flushexit(exitOSError)

def flushbuffers():
        sys.stdout.flush()
        sys.stderr.flush()

def flushexit(rc):
        flushbuffers()
        sys.exit(rc)

def flushkill(pid, sig):
        flushbuffers()
        os.kill(pid, sig)

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
            signum = os.WTERMSIG(status)
            if signum == 2:
                msg = 'INTERRUPT'
                rc = exitInterrupt
            else:
                msg = 'CRASHED'
                rc = exitSignal

            print "\n%s EXIT STATUS: %s signal %d %s (%f seconds)\n" % (prefix, msg, signum, getSignalName(signum), elapsedtime)
            flushexit(rc)

	elif os.WIFEXITED(status):
	    rc = os.WEXITSTATUS(status)
	    msg = ''
	    if rc == 0:
	        msg = 'NORMAL'
	    else:
	        msg = 'ABNORMAL ' + str(rc)
		rc = exitSignal

	    print "\n%s EXIT STATUS: %s (%f seconds)\n" % (prefix, msg, elapsedtime)
	    flushexit(rc)
	else:
	    print "\n%s EXIT STATUS: NONE (%f seconds)\n" % (prefix, elapsedtime)
	    flushexit(0)
except KeyboardInterrupt:
	flushkill(pid, 9)
	flushexit(exitInterrupt)


try:
    os.getpgid(pid)
    
    flushkill(pid, 9)
    flushexit(exitOSError)
except OSError:
    
    1
