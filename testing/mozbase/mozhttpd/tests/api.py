





import mozfile
import mozhttpd
import urllib2
import os
import unittest
import json
import tempfile

here = os.path.dirname(os.path.abspath(__file__))

class ApiTest(unittest.TestCase):
    resource_get_called = 0
    resource_post_called = 0
    resource_del_called = 0

    @mozhttpd.handlers.json_response
    def resource_get(self, request, objid):
        self.resource_get_called += 1
        return (200, { 'called': self.resource_get_called,
                       'id': objid,
                       'query': request.query })

    @mozhttpd.handlers.json_response
    def resource_post(self, request):
        self.resource_post_called += 1
        return (201, { 'called': self.resource_post_called,
                       'data': json.loads(request.body),
                       'query': request.query })

    @mozhttpd.handlers.json_response
    def resource_del(self, request, objid):
        self.resource_del_called += 1
        return (200, { 'called': self.resource_del_called,
                       'id': objid,
                       'query': request.query })

    def get_url(self, path, server_port, querystr):
        url = "http://127.0.0.1:%s%s" % (server_port, path)
        if querystr:
            url += "?%s" % querystr
        return url

    def try_get(self, server_port, querystr):
        self.resource_get_called = 0

        f = urllib2.urlopen(self.get_url('/api/resource/1', server_port, querystr))
        try:
            self.assertEqual(f.getcode(), 200)
        except AttributeError:
            pass  
        self.assertEqual(json.loads(f.read()), { 'called': 1, 'id': str(1), 'query': querystr })
        self.assertEqual(self.resource_get_called, 1)

    def try_post(self, server_port, querystr):
        self.resource_post_called = 0

        postdata = { 'hamburgers': '1234' }
        try:
            f = urllib2.urlopen(self.get_url('/api/resource/', server_port, querystr),
                                data=json.dumps(postdata))
        except urllib2.HTTPError, e:
            
            self.assertEqual(e.code, 201)
            body = e.fp.read()
        else:
            self.assertEqual(f.getcode(), 201)
            body = f.read()
        self.assertEqual(json.loads(body), { 'called': 1,
                                             'data': postdata,
                                             'query': querystr })
        self.assertEqual(self.resource_post_called, 1)

    def try_del(self, server_port, querystr):
        self.resource_del_called = 0

        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(self.get_url('/api/resource/1', server_port, querystr))
        request.get_method = lambda: 'DEL'
        f = opener.open(request)

        try:
            self.assertEqual(f.getcode(), 200)
        except AttributeError:
            pass  
        self.assertEqual(json.loads(f.read()), { 'called': 1, 'id': str(1), 'query': querystr })
        self.assertEqual(self.resource_del_called, 1)

    def test_api(self):
        httpd = mozhttpd.MozHttpd(port=0,
                                  urlhandlers = [ { 'method': 'GET',
                                                    'path': '/api/resource/([^/]+)/?',
                                                    'function': self.resource_get },
                                                  { 'method': 'POST',
                                                    'path': '/api/resource/?',
                                                    'function': self.resource_post },
                                                  { 'method': 'DEL',
                                                    'path': '/api/resource/([^/]+)/?',
                                                    'function': self.resource_del }
                                                  ])
        httpd.start(block=False)

        server_port = httpd.httpd.server_port

        
        self.try_get(server_port, '')
        self.try_get(server_port, '?foo=bar')

        
        self.try_post(server_port, '')
        self.try_post(server_port, '?foo=bar')

        
        self.try_del(server_port, '')
        self.try_del(server_port, '?foo=bar')

        
        exception_thrown = False
        try:
            urllib2.urlopen(self.get_url('/', server_port, None))
        except urllib2.HTTPError, e:
            self.assertEqual(e.code, 404)
            exception_thrown = True
        self.assertTrue(exception_thrown)

    def test_nonexistent_resources(self):
        
        
        httpd = mozhttpd.MozHttpd(port=0)
        httpd.start(block=False)
        server_port = httpd.httpd.server_port

        
        exception_thrown = False
        try:
            urllib2.urlopen(self.get_url('/api/resource/', server_port, None))
        except urllib2.HTTPError, e:
            self.assertEqual(e.code, 404)
            exception_thrown = True
        self.assertTrue(exception_thrown)

        
        exception_thrown = False
        try:
            urllib2.urlopen(self.get_url('/api/resource/', server_port, None),
                            data=json.dumps({}))
        except urllib2.HTTPError, e:
            self.assertEqual(e.code, 404)
            exception_thrown = True
        self.assertTrue(exception_thrown)

        
        exception_thrown = False
        try:
            opener = urllib2.build_opener(urllib2.HTTPHandler)
            request = urllib2.Request(self.get_url('/api/resource/', server_port,
                                                   None))
            request.get_method = lambda: 'DEL'
            opener.open(request)
        except urllib2.HTTPError:
            self.assertEqual(e.code, 404)
            exception_thrown = True
        self.assertTrue(exception_thrown)

    def test_api_with_docroot(self):
        httpd = mozhttpd.MozHttpd(port=0, docroot=here,
                                  urlhandlers = [ { 'method': 'GET',
                                                    'path': '/api/resource/([^/]+)/?',
                                                    'function': self.resource_get } ])
        httpd.start(block=False)
        server_port = httpd.httpd.server_port

        
        f = urllib2.urlopen(self.get_url('/', server_port, None))
        try:
            self.assertEqual(f.getcode(), 200)
        except AttributeError:
            pass  
        self.assertTrue('Directory listing for' in f.read())

        
        self.try_get(server_port, '')
        self.try_get(server_port, '?foo=bar')

class ProxyTest(unittest.TestCase):

    def tearDown(self):
        
        urllib2.install_opener(None)

    def test_proxy(self):
        docroot = tempfile.mkdtemp()
        self.addCleanup(mozfile.remove, docroot)
        hosts = ('mozilla.com', 'mozilla.org')
        unproxied_host = 'notmozilla.org'
        def url(host): return 'http://%s/' % host

        index_filename = 'index.html'
        def index_contents(host): return '%s index' % host

        index = file(os.path.join(docroot, index_filename), 'w')
        index.write(index_contents('*'))
        index.close()

        httpd = mozhttpd.MozHttpd(port=0, docroot=docroot)
        httpd.start(block=False)
        server_port = httpd.httpd.server_port

        proxy_support = urllib2.ProxyHandler({'http': 'http://127.0.0.1:%d' %
                                              server_port})
        urllib2.install_opener(urllib2.build_opener(proxy_support))

        for host in hosts:
            f = urllib2.urlopen(url(host))
            try:
                self.assertEqual(f.getcode(), 200)
            except AttributeError:
                pass  
            self.assertEqual(f.read(), index_contents('*'))

        httpd.stop()

        

        httpd = mozhttpd.MozHttpd(port=0, docroot=docroot, proxy_host_dirs=True)
        httpd.start(block=False)
        server_port = httpd.httpd.server_port

        proxy_support = urllib2.ProxyHandler({'http': 'http://127.0.0.1:%d' %
                                              server_port})
        urllib2.install_opener(urllib2.build_opener(proxy_support))

        
        for host in hosts:
            os.mkdir(os.path.join(docroot, host))
            file(os.path.join(docroot, host, index_filename), 'w') \
                .write(index_contents(host))

        for host in hosts:
            f = urllib2.urlopen(url(host))
            try:
                self.assertEqual(f.getcode(), 200)
            except AttributeError:
                pass  
            self.assertEqual(f.read(), index_contents(host))

        exc = None
        try:
            urllib2.urlopen(url(unproxied_host))
        except urllib2.HTTPError, e:
            exc = e
        self.assertNotEqual(exc, None)
        self.assertEqual(exc.code, 404)


if __name__ == '__main__':
    unittest.main()
