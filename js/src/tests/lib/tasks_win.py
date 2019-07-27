


from __future__ import print_function, unicode_literals, division

import subprocess
import sys
from datetime import datetime, timedelta
from progressbar import ProgressBar
from results import TestOutput
from threading import Thread
from Queue import Queue, Empty


class EndMarker:
    pass


class TaskFinishedMarker:
    pass


def _do_work(qTasks, qResults, qWatch, prefix, timeout):
    while True:
        test = qTasks.get(block=True, timeout=sys.maxint)
        if test is EndMarker:
            qWatch.put(EndMarker)
            qResults.put(EndMarker)
            return

        
        cmd = test.get_command(prefix)
        tStart = datetime.now()
        proc = subprocess.Popen(cmd,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

        
        
        
        qWatch.put(proc)
        out, err = proc.communicate()
        qWatch.put(TaskFinishedMarker)

        
        dt = datetime.now() - tStart
        result = TestOutput(test, cmd, out, err, proc.returncode, dt.total_seconds(),
                            dt > timedelta(seconds=timeout))
        qResults.put(result)


def _do_watch(qWatch, timeout):
    while True:
        proc = qWatch.get(True)
        if proc == EndMarker:
            return
        try:
            fin = qWatch.get(block=True, timeout=timeout)
            assert fin is TaskFinishedMarker, "invalid finish marker"
        except Empty:
            
            proc.terminate()
            fin = qWatch.get(block=True, timeout=sys.maxint)
            assert fin is TaskFinishedMarker, "invalid finish marker"


def run_all_tests(tests, prefix, results, options):
    """
    Uses scatter-gather to a thread-pool to manage children.
    """
    qTasks, qResults = Queue(), Queue()

    workers = []
    watchdogs = []
    for _ in range(options.worker_count):
        qWatch = Queue()
        watcher = Thread(target=_do_watch, args=(qWatch, options.timeout))
        watcher.setDaemon(True)
        watcher.start()
        watchdogs.append(watcher)
        worker = Thread(target=_do_work, args=(qTasks, qResults, qWatch,
                                               prefix, options.timeout))
        worker.setDaemon(True)
        worker.start()
        workers.append(worker)

    
    
    
    
    
    for test in tests:
        qTasks.put(test)
    for _ in workers:
        qTasks.put(EndMarker)

    
    ended = 0
    delay = ProgressBar.update_granularity().total_seconds()
    while ended < len(workers):
        try:
            result = qResults.get(block=True, timeout=delay)
            if result is EndMarker:
                ended += 1
            else:
                yield result
        except Empty:
            results.pb.poke()

    
    for worker in workers:
        worker.join()
    for watcher in watchdogs:
        watcher.join()
    assert qTasks.empty(), "Send queue not drained"
    assert qResults.empty(), "Result queue not drained"
