



from marionette import MarionetteTestCase

class TestFail(MarionetteTestCase):
    def test_fails(self):
        
        self.assertEquals(True, False)
