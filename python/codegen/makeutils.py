



import errno

dependencies = []
targets = []

def makeQuote(filename):
    return filename.replace(' ', '\\ ')  

def writeMakeDependOutput(filename):
    print "Creating makedepend file", filename
    dir = os.path.dirname(filename)
    if dir and not os.path.exists(dir):
        try:
            os.makedirs(dir)
        except OSError as error:
            if error.errno != errno.EEXIST:
                raise

    with open(filename, 'w') as f:
        if len(targets) > 0:
            f.write("%s:" % makeQuote(targets[0]))
            for filename in dependencies:
                f.write(' \\\n\t\t%s' % makeQuote(filename))
            f.write('\n\n')
            for filename in targets[1:]:
                f.write('%s: %s\n' % (makeQuote(filename), makeQuote(targets[0])))

