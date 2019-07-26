import fcntl, os, select, time
from subprocess import Popen, PIPE











class TaskPool(object):

    
    
    class Task(object):
        def __init__(self):
            self.pipe = None
            self.start_time = None

        
        
        def start(self, pipe, deadline):
            self.pipe = pipe
            self.deadline = deadline

        
        
        
        
        def cmd(self):
            raise NotImplementedError

        
        
        def onStdout(self, string):
            raise NotImplementedError

        
        
        def onStderr(self, string):
            raise NotImplementedError

        
        
        def onFinished(self, returncode):
            raise NotImplementedError

        
        
        def onTimeout(self):
            raise NotImplementedError

    
    
    class TerminateTask(Exception):
        pass

    def __init__(self, tasks, cwd='.', job_limit=4, timeout=150):
        self.pending = iter(tasks)
        self.cwd = cwd
        self.job_limit = job_limit
        self.timeout = timeout
        self.next_pending = self.get_next_pending()

    
    def get_next_pending(self):
        try:
            return self.pending.next()
        except StopIteration:
            return None

    def run_all(self):
        
        running = set()
        with open(os.devnull, 'r') as devnull:
            while True:
                while len(running) < self.job_limit and self.next_pending:
                    t = self.next_pending
                    p = Popen(t.cmd(), bufsize=16384,
                              stdin=devnull, stdout=PIPE, stderr=PIPE,
                              cwd=self.cwd)

                    
                    
                    flags = fcntl.fcntl(p.stdout, fcntl.F_GETFL)
                    fcntl.fcntl(p.stdout, fcntl.F_SETFL, flags | os.O_NONBLOCK)
                    flags = fcntl.fcntl(p.stderr, fcntl.F_GETFL)
                    fcntl.fcntl(p.stderr, fcntl.F_SETFL, flags | os.O_NONBLOCK)

                    t.start(p, time.time() + self.timeout)
                    running.add(t)
                    self.next_pending = self.get_next_pending()

                
                
                if not running:
                    break

                
                now = time.time()
                secs_to_next_deadline = max(min([t.deadline for t in running]) - now, 0)

                
                stdouts_and_stderrs = ([t.pipe.stdout for t in running]
                                     + [t.pipe.stderr for t in running])
                (readable,w,x) = select.select(stdouts_and_stderrs, [], [], secs_to_next_deadline)
                finished = set()
                terminate = set()
                for t in running:
                    
                    
                    
                    
                    
                    
                    if t.pipe.stdout in readable:
                        output = t.pipe.stdout.read(16384)
                        if output != "":
                            try:
                                t.onStdout(output)
                            except TerminateTask:
                                terminate.add(t)
                    if t.pipe.stderr in readable:
                        output = t.pipe.stderr.read(16384)
                        if output != "":
                            try:
                                t.onStderr(output)
                            except TerminateTask:
                                terminate.add(t)
                        else:
                            
                            
                            
                            t.pipe.wait()
                            t.onFinished(t.pipe.returncode)
                            finished.add(t)
                
                
                running -= finished

                
                for t in terminate:
                    t.pipe.terminate()
                    t.pipe.wait()
                    running.remove(t)

                
                finished = set()
                for t in running:
                    if now >= t.deadline:
                        t.pipe.terminate()
                        t.pipe.wait()
                        t.onTimeout()
                        finished.add(t)
                
                
                running -= finished
        return None

def get_cpu_count():
    """
    Guess at a reasonable parallelism count to set as the default for the
    current machine and run.
    """
    
    try:
        import multiprocessing
        return multiprocessing.cpu_count()
    except (ImportError,NotImplementedError):
        pass

    
    try:
        res = int(os.sysconf('SC_NPROCESSORS_ONLN'))
        if res > 0:
            return res
    except (AttributeError,ValueError):
        pass

    
    try:
        res = int(os.environ['NUMBER_OF_PROCESSORS'])
        if res > 0:
            return res
    except (KeyError, ValueError):
        pass

    return 1

if __name__ == '__main__':
    
    def sleep_sort(ns, timeout):
        sorted=[]
        class SortableTask(TaskPool.Task):
            def __init__(self, n):
                super(SortableTask, self).__init__()
                self.n = n
            def start(self, pipe, deadline):
                super(SortableTask, self).start(pipe, deadline)
            def cmd(self):
                return ['sh', '-c', 'echo out; sleep %d; echo err>&2' % (self.n,)]
            def onStdout(self, text):
                print '%d stdout: %r' % (self.n, text)
            def onStderr(self, text):
                print '%d stderr: %r' % (self.n, text)
            def onFinished(self, returncode):
                print '%d (rc=%d)' % (self.n, returncode)
                sorted.append(self.n)
            def onTimeout(self):
                print '%d timed out' % (self.n,)

        p = TaskPool([SortableTask(_) for _ in ns], job_limit=len(ns), timeout=timeout)
        p.run_all()
        return sorted

    print repr(sleep_sort([1,1,2,3,5,8,13,21,34], 15))
