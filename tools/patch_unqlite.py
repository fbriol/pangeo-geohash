import sys
import re

PATTERN = re.compile(r'^(\s+)(pgno\s+pgno;)\s{2}(\s+.*)$').search


def main():
    with open(sys.argv[1], "r") as stream:
        lines = stream.readlines()
    for ix, line in enumerate(lines):
        m = PATTERN(line)
        if m is not None:
            lines[ix] = m.group(1) + "::" + m.group(2) + m.group(3) + "\n"

    with open(sys.argv[2], "w") as stream:
        stream.writelines(lines)


if __name__ == "__main__":
    main()
