




from __future__ import print_function

import errno
import os
import subprocess




ANDROID_PLATFORM = 'android-21'
ANDROID_BUILD_TOOLS_VERSION = '21.1.2'



ANDROID_PACKAGES = [
    'tools',
    'platform-tools',
    'build-tools-%s' % ANDROID_BUILD_TOOLS_VERSION,
    ANDROID_PLATFORM,
    'extra-android-support',
    'extra-google-google_play_services',
    'extra-google-m2repository',
    'extra-android-m2repository',
]

ANDROID_NDK_EXISTS = '''
Looks like you have the Android NDK installed at:
%s
'''

ANDROID_SDK_EXISTS = '''
Looks like you have the Android SDK installed at:
%s
We will install all required Android packages.
'''

NOT_INSTALLING_ANDROID_PACKAGES = '''
It looks like you already have the following Android packages:
%s
No need to update!
'''

INSTALLING_ANDROID_PACKAGES = '''
We are now installing the following Android packages:
%s
You may be prompted to agree to the Android license. You may see some of
output as packages are downloaded and installed.
'''

MISSING_ANDROID_PACKAGES = '''
We tried to install the following Android packages:
%s
But it looks like we couldn't install:
%s
Install these Android packages manually and run this bootstrapper again.
'''

MOBILE_ANDROID_MOZCONFIG_TEMPLATE = '''
Paste the lines between the chevrons (>>> and <<<) into your mozconfig file:

<<<
# Build Firefox for Android:
ac_add_options --enable-application=mobile/android
ac_add_options --target=arm-linux-androideabi

# With the following Android SDK and NDK:
ac_add_options --with-android-sdk="%s"
ac_add_options --with-android-ndk="%s"
>>>
'''


def check_output(*args, **kwargs):
    """Run subprocess.check_output even if Python doesn't provide it."""
    from base import BaseBootstrapper
    fn = getattr(subprocess, 'check_output', BaseBootstrapper._check_output)

    return fn(*args, **kwargs)

def list_missing_android_packages(android_tool, packages):
    '''
    Use the given |android| tool to return the sub-list of Android
    |packages| given that are not installed.
    '''
    missing = []

    
    
    
    lines = check_output([android_tool,
        'list', 'sdk', '--no-ui', '--extended']).splitlines()

    
    for line in lines:
        is_id_line = False
        try:
            is_id_line = line.startswith("id:")
        except:
            
            pass
        if not is_id_line:
            continue

        for package in packages:
            if '"%s"' % package in line:
                
                missing.append(package)

    return missing

def install_mobile_android_sdk_or_ndk(url, path):
    '''
    Fetch an Android SDK or NDK from |url| and unpack it into
    the given |path|.

    We expect wget to be installed and found on the system path.

    We use, and wget respects, https.  We could also include SHAs for a
    small improvement in the integrity guarantee we give. But this script is
    bootstrapped over https anyway, so it's a really minor improvement.

    We use |wget --continue| as a cheap cache of the downloaded artifacts,
    writing into |path|/mozboot.  We don't yet clean the cache; it's better
    to waste disk and not require a long re-download than to wipe the cache
    prematurely.
    '''

    old_path = os.getcwd()
    try:
        download_path = os.path.join(path, 'mozboot')
        try:
            os.makedirs(download_path)
        except OSError as e:
            if e.errno == errno.EEXIST and os.path.isdir(download_path):
                pass
            else:
                raise

        os.chdir(download_path)
        subprocess.check_call(['wget', '--continue', url])
        file = url.split('/')[-1]

        os.chdir(path)
        if file.endswith('.tar.gz') or file.endswith('.tgz'):
            cmd = ['tar', 'zvxf']
        elif file.endswith('.tar.bz2'):
            cmd = ['tar', 'jvxf']
        elif file.endswitch('.zip'):
            cmd = ['unzip']
        else:
            raise NotImplementedError("Don't know how to unpack file: %s" % file)
        subprocess.check_call(cmd + [os.path.join(download_path, file)])
    finally:
        os.chdir(old_path)

def ensure_android_sdk_and_ndk(path, sdk_path, sdk_url, ndk_path, ndk_url):
    '''
    Ensure the Android SDK and NDK are found at the given paths.  If not, fetch
    and unpack the SDK and/or NDK from the given URLs into |path|.
    '''

    
    
    
    if os.path.isdir(ndk_path):
        print(ANDROID_NDK_EXISTS % ndk_path)
    else:
        install_mobile_android_sdk_or_ndk(ndk_url, path)

    
    
    
    if os.path.isdir(sdk_path):
        print(ANDROID_SDK_EXISTS % sdk_path)
    else:
        install_mobile_android_sdk_or_ndk(sdk_url, path)

def ensure_android_packages(android_tool, packages=None):
    '''
    Use the given android tool (like 'android') to install required Android
    packages.
    '''

    if not packages:
        packages = ANDROID_PACKAGES

    missing = list_missing_android_packages(android_tool, packages=packages)
    if not missing:
        print(NOT_INSTALLING_ANDROID_PACKAGES % ', '.join(packages))
        return

    
    
    print(INSTALLING_ANDROID_PACKAGES % ', '.join(missing))
    subprocess.check_call([android_tool,
        'update', 'sdk', '--no-ui',
        '--filter', ','.join(missing)])

    
    failing = list_missing_android_packages(android_tool, packages=packages)
    if failing:
        raise Exception(MISSING_ANDROID_PACKAGES % (', '.join(missing), ', '.join(failing)))

def suggest_mozconfig(sdk_path=None, ndk_path=None):
    print(MOBILE_ANDROID_MOZCONFIG_TEMPLATE % (sdk_path, ndk_path))
