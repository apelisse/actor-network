# -*- coding: utf-8 -*-

import re
import sys


def parse_file(f):
    fp = open(f)

    while not fp.readline().endswith('S LIST\n'):
        pass

    for _ in range(4):
        fp.readline()

    pattern = re.compile(r"""
(?P<actor>[^\t]*)\t*
(?P<movie>.+?)\s
\((?P<year>[\d?]{4})(/[IVX]+)?\)
(\s*\((?P<type>(V|TV|VG))\))?
[^{]*(?P<episode>\{.[^}]+\})?.*
""", re.X)

    for line in fp:
        if not line.strip():
            continue

        if line.startswith('---------------------------------------------'):
            break

        m = pattern.match(line)
        if not m:
            print >> sys.stderr, "error can't parse line:", line.rstrip()
            continue

        actor = m.group("actor")
        movie = m.group("movie")
        episode = m.group("episode")
        year = m.group("year")
        foundtype = m.group("type")
        if actor:
            print 'actor', actor
        if movie:
            if episode:
                print 'movie', movie, episode, year
            else:
                print 'movie', movie, year

            # Compute the type
            if movie[0] == '"':
                movietype = "series"
            elif foundtype == "V":
                movietype = "video"
            elif foundtype == "VG":
                movietype = "game"
            elif foundtype == "TV":
                movietype = "tv"
            else:
                movietype = "movie"
            print 'type', movietype


def main(files):
    for f in files:
        parse_file(f)


if __name__ == '__main__':
    main(sys.argv[1:])
