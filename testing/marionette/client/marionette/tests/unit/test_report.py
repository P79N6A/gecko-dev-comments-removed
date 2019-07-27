



from marionette import MarionetteTestCase, expectedFailure, skip


class TestReport(MarionetteTestCase):

    def test_pass(self):
        assert True

    def test_fail(self):
        assert False

    @skip('Skip Message')
    def test_skip(self):
        assert False

    @expectedFailure
    def test_expected_fail(self):
        assert False

    @expectedFailure
    def test_unexpected_pass(self):
        assert True

    def test_error(self):
        raise Exception()
