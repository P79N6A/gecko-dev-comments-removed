


from __future__ import print_function, unicode_literals, division

import sys
from threading import Thread
from Queue import Queue, Empty


class EndMarker:
    pass


def _do_work(qTasks, qResults, timeout):
    while True:
        test = qTasks.get(block=True, timeout=sys.maxint)
        if test is EndMarker:
            qResults.put(EndMarker)
            return
        qResults.put(test.run(prefix, timeout))


def run_all_tests_gen(tests, prefix, results, options):
    """
    Uses scatter-gather to a thread-pool to manage children.
    """
    qTasks, qResults = Queue(), Queue()

    workers = []
    for _ in range(options.worker_count):
        worker = Thread(target=_do_work, args=(qTasks, qResults, prefix,
                                               options.timeout))
        worker.setDaemon(True)
        worker.start()
        workers.append(worker)

    
    
    
    
    
    for test in tests:
        qTasks.put(test)
    for _ in workers:
        qTasks.put(EndMarker)

    
    ended = 0
    while ended < len(workers):
        result = qResults.get(block=True, timeout=sys.maxint)
        if result is EndMarker:
            ended += 1
        else:
            yield result

    
    for worker in workers:
        worker.join()
    assert qTasks.empty(), "Send queue not drained"
    assert qResults.empty(), "Result queue not drained"


def run_all_tests(tests, results, options):
    for result in run_all_tests_gen(tests, results, options):
        results.push(result)
    return True

