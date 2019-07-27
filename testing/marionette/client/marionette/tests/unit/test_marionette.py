



from marionette_driver import errors
import marionette_test


class TestHandleError(marionette_test.MarionetteTestCase):
    def test_malformed_packet(self):
        for t in [{}, {"error": None}]:
            with self.assertRaisesRegexp(errors.MarionetteException, "Malformed packet"):
                self.marionette._handle_error(t)

    def test_known_error_code(self):
        with self.assertRaises(errors.NoSuchElementException):
            self.marionette._handle_error(
                {"error": {"status": errors.ErrorCodes.NO_SUCH_ELEMENT}})

    def test_unknown_error_code(self):
        with self.assertRaises(errors.MarionetteException):
            self.marionette._handle_error({"error": {"status": 123456}})
