

import os
import time
import unittest
import proctest
from mozprocess import processhandler

here = os.path.dirname(os.path.abspath(__file__))

class ProcTestKill(proctest.ProcTest):
    """ Class to test various process tree killing scenatios """

    
    
    
    def test_process_kill_broad_wait(self):
        """Process is started, we use a broad process tree, we let it spawn
           for a bit, we kill it"""

        p = processhandler.ProcessHandler([self.python, self.proclaunch, "process_normal_broad_python.ini"],
                                          cwd=here)
        p.run()
        
        time.sleep(3)
        p.kill()

        detected, output = proctest.check_for_process(self.proclaunch)
        self.determine_status(detected,
                              output,
                              p.proc.returncode,
                              p.didTimeout)

if __name__ == '__main__':
    unittest.main()
