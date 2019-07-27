



from .base import TestExecutor


class ProcessTestExecutor(TestExecutor):
    def __init__(self, *args, **kwargs):
        TestExecutor.__init__(self, *args, **kwargs)
        self.binary = self.browser.binary
        self.interactive = self.browser.interactive

    def setup(self, runner):
        self.runner = runner
        self.runner.send_message("init_succeeded")
        return True

    def is_alive(self):
        return True

    def do_test(self, test):
        raise NotImplementedError
