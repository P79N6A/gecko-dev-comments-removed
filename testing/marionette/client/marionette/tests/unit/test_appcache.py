














from marionette_driver.application_cache import ApplicationCache
from marionette_test import MarionetteTestCase


class AppCacheTests(MarionetteTestCase):

    def testWeCanGetTheStatusOfTheAppCache(self):
        test_url = self.marionette.absolute_url('html5Page')
        self.marionette.navigate(test_url)
        app_cache = self.marionette.application_cache

        status = app_cache.status
        while status == ApplicationCache.DOWNLOADING:
            status = app_cache.status

        self.assertEquals(ApplicationCache.UNCACHED, app_cache.status)
