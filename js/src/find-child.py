
















from mercurial import ui, hg
from hgext.convert.filemap import filemapper
from optparse import OptionParser


parser = OptionParser()

parser.add_option("-s", "--src", dest="src",
                  help="source repository", metavar="REPO")

parser.add_option("-d", "--dst", dest="dst",
                  help="destination repository", metavar="REPO")

parser.add_option("-t", "--start", dest="start",
                  help="starting revid in source repository", metavar="REV")

parser.add_option("-f", "--filemap", dest="filemap",
                  help="filemap used in conversion", metavar="PATH")

(options, args) = parser.parse_args()

if not (options.src and options.dst and options.start):
    parser.print_help()
    exit(1)

u = ui.ui()

src_repo = hg.repository(u, options.src)
dst_repo = hg.repository(u, options.dst)

fm = None
if options.filemap:
    fm = filemapper(u, options.filemap)

last_converted_src = src_repo[options.start]

dst_tip = dst_repo.changectx(dst_repo.changelog.tip()).hex()
revs = last_converted_src.children()

while len(revs) != 0:
    tmp = revs
    revs = []
    for child in tmp:
        for f in child.files():
            if (not fm) or fm(f):
                u.write("%s %s\n" % (child.hex(), dst_tip))
                exit(0);
        revs.extend(child.children())
