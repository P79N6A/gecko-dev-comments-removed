



import os

"""
Adds touch support in Marionette
"""
class MarionetteTouchMixin(object):
    """
    Set up the touch layer.
    Can specify another library with a path and the name of the library.
    """
    def setup_touch(self, library=None, library_name=None):
        self.library = library or os.path.abspath(os.path.join(__file__, os.pardir, "touch", "synthetic_gestures.js"))
        self.library_name = library_name or "SyntheticGestures"
        self.import_script(self.library)

    def tap(self, element):
        self.execute_script("%s.tap(arguments[0]);" % self.library_name, [element])

    def double_tap(self, element):
        self.execute_script("%s.dbltap(arguments[0]);" % self.library_name, [element])

    def long_press(self, element, holdtime=2000):
        
        self.execute_script("%s.hold.apply(this, arguments);" % self.library_name, [element, holdtime, 0, 0, 0, 0, 0])
        
    def flick(self, element, x1, y1, x2, y2, duration=200):
        
        self.execute_script("%s.swipe.apply(this, arguments);" % self.library_name, [element, x1, y1, x2, y2, duration])

    def pinch(self, *args, **kwargs):
        raise Exception("Pinch is unsupported")
