



'''
Replace localized parts of a packaged directory with data from a langpack
directory.
'''

import os
import mozpack.path as mozpath
from mozpack.packager.formats import (
    FlatFormatter,
    JarFormatter,
    OmniJarFormatter,
)
from mozpack.packager import (
    Component,
    SimplePackager,
    SimpleManifestSink,
)
from mozpack.files import (
    ComposedFinder,
    ManifestFile,
)
from mozpack.copier import (
    FileCopier,
    Jarrer,
)
from mozpack.chrome.manifest import (
    ManifestLocale,
    ManifestEntryWithRelPath,
    is_manifest,
    ManifestChrome,
    Manifest,
)
from mozpack.errors import errors
from mozpack.packager.unpack import UnpackFinder
from createprecomplete import generate_precomplete


class LocaleManifestFinder(object):
    def __init__(self, finder):
        entries = self.entries = []
        bases = self.bases = ['']

        class MockFormatter(object):
            def add_interfaces(self, path, content):
                pass

            def add(self, path, content):
                pass

            def add_manifest(self, entry):
                if entry.localized:
                    entries.append(entry)

            def add_base(self, base, addon=False):
                bases.append(base)

        
        
        
        
        
        class WrapFinder(object):
            def __init__(self, finder):
                self._finder = finder

            def find(self, pattern):
                for p, f in self._finder.find(pattern):
                    if isinstance(f, ManifestFile):
                        unwanted = [
                            e for e in f._entries
                            if isinstance(e, Manifest) and e.flags
                        ]
                        if unwanted:
                            f = ManifestFile(
                                f._base,
                                [e for e in f._entries if e not in unwanted])
                    yield p, f

        sink = SimpleManifestSink(WrapFinder(finder), MockFormatter())
        sink.add(Component(''), '*')
        sink.close(False)

        
        self.locales = list(set(e.id for e in self.entries
                                if isinstance(e, ManifestLocale)))


def _repack(app_finder, l10n_finder, copier, formatter, non_chrome=set()):
    app = LocaleManifestFinder(app_finder)
    l10n = LocaleManifestFinder(l10n_finder)

    
    
    if len(app.locales) > 1 or len(l10n.locales) > 1:
        errors.fatal("Multiple locales aren't supported")
    locale = app.locales[0]
    l10n_locale = l10n.locales[0]

    
    
    
    
    
    
    l10n_paths = {}
    for e in l10n.entries:
        if isinstance(e, ManifestChrome):
            base = mozpath.basedir(e.path, app.bases)
            l10n_paths.setdefault(base, {})
            l10n_paths[base][e.name] = e.path

    
    
    paths = {}
    for e in app.entries:
        if isinstance(e, ManifestEntryWithRelPath):
            base = mozpath.basedir(e.path, app.bases)
            if base not in l10n_paths:
                errors.fatal("Locale doesn't contain %s/" % base)
                
                continue
            if e.name not in l10n_paths[base]:
                errors.fatal("Locale doesn't have a manifest entry for '%s'" %
                    e.name)
                
                continue
            paths[e.path] = l10n_paths[base][e.name]

    for pattern in non_chrome:
        for base in app.bases:
            path = mozpath.join(base, pattern)
            left = set(p for p, f in app_finder.find(path))
            right = set(p for p, f in l10n_finder.find(path))
            for p in right:
                paths[p] = p
            for p in left - right:
                paths[p] = None

    
    
    packager = SimplePackager(formatter)
    for p, f in app_finder:
        if is_manifest(p):
            
            for e in [e for e in f if e.localized]:
                f.remove(e)
        
        
        path = None
        if p in paths:
            path = paths[p]
            if not path:
                continue
        else:
            base = mozpath.basedir(p, paths.keys())
            if base:
                subpath = mozpath.relpath(p, base)
                path = mozpath.normpath(mozpath.join(paths[base],
                                                               subpath))
        if path:
            files = [f for p, f in l10n_finder.find(path)]
            if not len(files):
                if base not in non_chrome:
                    errors.error("Missing file: %s" %
                                 os.path.join(l10n_finder.base, path))
            else:
                packager.add(path, files[0])
        else:
            packager.add(p, f)

    
    l10n_manifests = []
    for base in set(e.base for e in l10n.entries):
        m = ManifestFile(base, [e for e in l10n.entries if e.base == base])
        path = mozpath.join(base, 'chrome.%s.manifest' % l10n_locale)
        l10n_manifests.append((path, m))
    bases = packager.get_bases()
    for path, m in l10n_manifests:
        base = mozpath.basedir(path, bases)
        packager.add(path, m)
        
        m = ManifestFile(base)
        m.add(Manifest(base, mozpath.relpath(path, base)))
        packager.add(mozpath.join(base, 'chrome.manifest'), m)

    packager.close()

    
    for pattern in non_chrome:
        for base in bases:
            for p, f in l10n_finder.find(mozpath.join(base, pattern)):
                if not formatter.contains(p):
                    formatter.add(p, f)

    
    for path, log in app_finder.jarlogs.iteritems():
        assert isinstance(copier[path], Jarrer)
        copier[path].preload([l.replace(locale, l10n_locale) for l in log])


def repack(source, l10n, extra_l10n={}, non_resources=[], non_chrome=set()):
    '''
    Replace localized data from the `source` directory with localized data
    from `l10n` and `extra_l10n`.

    The `source` argument points to a directory containing a packaged
    application (in omnijar, jar or flat form).
    The `l10n` argument points to a directory containing the main localized
    data (usually in the form of a language pack addon) to use to replace
    in the packaged application.
    The `extra_l10n` argument contains a dict associating relative paths in
    the source to separate directories containing localized data for them.
    This can be used to point at different language pack addons for different
    parts of the package application.
    The `non_resources` argument gives a list of relative paths in the source
    that should not be added in an omnijar in case the packaged application
    is in that format.
    The `non_chrome` argument gives a list of file/directory patterns for
    localized files that are not listed in a chrome.manifest.
    '''
    app_finder = UnpackFinder(source)
    l10n_finder = UnpackFinder(l10n)
    if extra_l10n:
        finders = {
            '': l10n_finder,
        }
        for base, path in extra_l10n.iteritems():
            finders[base] = UnpackFinder(path)
        l10n_finder = ComposedFinder(finders)
    copier = FileCopier()
    if app_finder.kind == 'flat':
        formatter = FlatFormatter(copier)
    elif app_finder.kind == 'jar':
        formatter = JarFormatter(copier, optimize=app_finder.optimizedjars)
    elif app_finder.kind == 'omni':
        formatter = OmniJarFormatter(copier, app_finder.omnijar,
                                     optimize=app_finder.optimizedjars,
                                     non_resources=non_resources)

    with errors.accumulate():
        _repack(app_finder, l10n_finder, copier, formatter, non_chrome)
    copier.copy(source, skip_if_older=False)
    generate_precomplete(source)
