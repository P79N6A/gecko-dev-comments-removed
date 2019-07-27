








def parseLocales(content):
    return sorted(l.split()[0] for l in content.splitlines() if l)
