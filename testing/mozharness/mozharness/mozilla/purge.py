





"""Purge/clobber support
"""



import os
import mozharness
external_tools_path = os.path.join(
    os.path.abspath(os.path.dirname(os.path.dirname(mozharness.__file__))),
    'external_tools',
)

from mozharness.base.log import ERROR





class PurgeMixin(object):
    purge_tool = os.path.join(external_tools_path, 'purge_builds.py')
    clobber_tool = os.path.join(external_tools_path, 'clobberer.py')

    default_skips = ['info', 'rel-*', 'tb-rel-*']
    default_maxage = 14
    default_periodic_clobber = 7 * 24

    def purge_builds(self, basedirs=None, min_size=None, skip=None, max_age=None):
        
        c = self.config
        dirs = self.query_abs_dirs()
        if 'clobberer_url' in c and c.get('use_clobberer', True):
            self.clobberer()

        min_size = min_size or c['purge_minsize']
        max_age = max_age or c.get('purge_maxage') or self.default_maxage
        skip = skip or c.get('purge_skip') or self.default_skips

        if not basedirs:
            
            
            basedirs = []
            basedirs.append(os.path.dirname(dirs['base_work_dir']))
            if self.config.get('purge_basedirs'):
                basedirs.extend(self.config.get('purge_basedirs'))

        cmd = []
        if self._is_windows():
            
            cmd.append(self.query_exe('python'))
        
        cmd.extend([self.purge_tool, '-s', str(min_size)])

        if max_age:
            cmd.extend(['--max-age', str(max_age)])

        for s in skip:
            cmd.extend(['--not', s])

        cmd.extend(basedirs)

        
        
        env = {'PATH': os.environ.get('PATH')}
        share_base = c.get('vcs_share_base', os.environ.get("HG_SHARE_BASE_DIR", None))
        if share_base:
            env['HG_SHARE_BASE_DIR'] = share_base
        retval = self.run_command(cmd, env=env)
        if retval != 0:
            self.fatal("failed to purge builds", exit_code=2)

    def clobberer(self):
        c = self.config
        dirs = self.query_abs_dirs()
        if not self.buildbot_config:
            self.fatal("clobberer requires self.buildbot_config (usually from $PROPERTIES_FILE)")

        periodic_clobber = c.get('periodic_clobber') or self.default_periodic_clobber
        clobberer_url = c['clobberer_url']

        builddir = os.path.basename(dirs['base_work_dir'])
        branch = self.buildbot_config['properties']['branch']
        buildername = self.buildbot_config['properties']['buildername']
        slave = self.buildbot_config['properties']['slavename']
        master = self.buildbot_config['properties']['master']

        cmd = []
        if self._is_windows():
            
            cmd.append(self.query_exe('python'))
        
        cmd.extend([self.clobber_tool])
        
        cmd.extend(['-s', 'scripts'])
        cmd.extend(['-s', 'logs'])
        cmd.extend(['-s', 'buildprops.json'])
        cmd.extend(['-s', 'token'])
        cmd.extend(['-s', 'oauth.txt'])

        if periodic_clobber:
            cmd.extend(['-t', str(periodic_clobber)])

        cmd.extend([clobberer_url, branch, buildername, builddir, slave, master])
        error_list = [{
            'substr': 'Error contacting server', 'level': ERROR,
            'explanation': 'Error contacting server for clobberer information.'
        }]

        retval = self.retry(self.run_command, attempts=3, good_statuses=(0,), args=[cmd],
                 kwargs={'cwd':os.path.dirname(dirs['base_work_dir']),
                         'error_list':error_list})
        if retval != 0:
            self.fatal("failed to clobber build", exit_code=2)

    def clobber(self, always_clobber_dirs=None):
        """ Mozilla clobberer-type clobber.
            """
        c = self.config
        if c.get('developer_mode'):
            self.info("Suppressing clobber in developer mode for safety.")
            return
        if c.get('is_automation'):
            
            do_clobber = False
            if self.query_is_nightly():
                self.info("Clobbering because we're a nightly build")
                do_clobber = True
            if c.get('force_clobber'):
                self.info("Clobbering because our config forced us to")
                do_clobber = True
            if do_clobber:
                super(PurgeMixin, self).clobber()
            else:
                
                
                if always_clobber_dirs is None:
                    always_clobber_dirs = []
                for path in always_clobber_dirs:
                    self.rmtree(path)
            
            self.purge_builds()
        else:
            super(PurgeMixin, self).clobber()
