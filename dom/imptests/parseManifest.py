



















def parseManifest(fd):
    def parseReftestLine(chunks):
        assert len(chunks) % 2 == 0
        reftests = []
        for i in range(2, len(chunks), 2):
            if not chunks[i] in ["==", "!="]:
                raise Exception("Misformatted reftest line " + line)
            reftests.append([chunks[i], chunks[1], chunks[i + 1]])
        return reftests

    dirs = []
    autotests = []
    reftests = []
    othertests = []
    supportfiles = []
    for fullline in fd:
        line = fullline.strip()
        if not line:
            continue

        chunks = line.split(" ")

        if chunks[0] == "MANIFEST":
            raise Exception("MANIFEST listed on line " + line)

        if chunks[0] == "dir" or (chunks[0] == "support" and chunks[1] == "dir"):
            dirs.append(chunks[1]);
        elif chunks[0] == "ref":
            if len(chunks) % 2:
                raise Exception("Missing chunk in line " + line)
            reftests.extend(parseReftestLine(chunks))
        elif chunks[0] == "support":
            supportfiles.append(chunks[1])
        elif chunks[0] in ["manual", "parser"]:
            othertests.append(chunks[1])
        else: 
            autotests.append(chunks[0])
    return dirs, autotests, reftests, othertests, supportfiles

def parseManifestFile(path):
    fp = open(path)
    dirs, autotests, reftests, othertests, supportfiles = parseManifest(fp)
    fp.close()
    return dirs, autotests, reftests, othertests, supportfiles
