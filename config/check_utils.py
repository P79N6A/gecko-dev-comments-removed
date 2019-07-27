




import subprocess

def get_all_toplevel_filenames():
    '''Get a list of all the files in the (Mercurial or Git) repository.'''
    try:
        cmd = ['hg', 'manifest', '-q']
        all_filenames = subprocess.check_output(cmd, universal_newlines=True,
                                                stderr=subprocess.PIPE).split('\n')
        return all_filenames
    except:
        pass

    try:
        
        cmd = ['git', 'rev-parse', '--show-cdup']
        top_level = subprocess.check_output(cmd, universal_newlines=True,
                                                stderr=subprocess.PIPE).split('\n')[0]
        cmd = ['git', 'ls-files', '--full-name', top_level]
        all_filenames = subprocess.check_output(cmd, universal_newlines=True,
                                                stderr=subprocess.PIPE).split('\n')
        return all_filenames
    except:
        pass

    raise Exception('failed to run any of the repo manifest commands', cmds)
