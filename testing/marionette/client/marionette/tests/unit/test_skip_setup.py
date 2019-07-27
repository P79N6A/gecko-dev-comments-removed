



from marionette import MarionetteTestCase, SkipTest

class TestSetUpSkipped(MarionetteTestCase):

    testVar = {'test':'SkipTest'}

    def setUp(self):
        try:
            self.testVar['email']
        except KeyError:
            raise SkipTest('email key not present in dict, skip ...')
        MarionetteTestCase.setUp(self)

    def test_assert(self):
        assert True

class TestSetUpNotSkipped(MarionetteTestCase):

    testVar = {'test':'SkipTest'}

    def setUp(self):
        try:
            self.testVar['test']
        except KeyError:
            raise SkipTest('email key not present in dict, skip ...')
        MarionetteTestCase.setUp(self)

    def test_assert(self):
        assert True

