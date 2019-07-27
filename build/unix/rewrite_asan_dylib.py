



import sys
import os
import subprocess
import shutil

'''
Scans the given directories for binaries referencing the AddressSanitizer
runtime library, copies it to the main directory and rewrites binaries to not
reference it with absolute paths but with @executable_path instead.
'''


DYLIB_NAME='libclang_rt.asan_osx_dynamic.dylib'

def scan_directory(path):
    dylibCopied = False

    for root, subdirs, files in os.walk(path):
        for filename in files:
            filename = os.path.join(root, filename)

            
            if not (filename.endswith('.dylib') or os.access(filename, os.X_OK)):
                continue

            try:
                otoolOut = subprocess.check_output(['otool', '-L', filename])
            except:
                
                continue

            for line in otoolOut.splitlines():
                if line.find(DYLIB_NAME) != -1:
                    absDylibPath = line.split()[0]

                    
                    if absDylibPath.find('@executable_path/') == 0:
                        continue

                    if not dylibCopied:
                        
                        
                        shutil.copy(absDylibPath, path)

                        
                        subprocess.check_call(['install_name_tool', '-id', '@executable_path/' + DYLIB_NAME, os.path.join(path, DYLIB_NAME)])
                        dylibCopied = True

                    
                    subprocess.check_call(['install_name_tool', '-change', absDylibPath, '@executable_path/' + DYLIB_NAME, filename])
                    break

if __name__ == '__main__':
    for d in sys.argv[1:]:
        scan_directory(d)
