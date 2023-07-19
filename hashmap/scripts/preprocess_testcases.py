import struct

# This wordlist is part of 'SecList' and can
# be found here:
# https://github.com/danielmiessler/SecLists/blob/master/Miscellaneous/lang-english.txt
FILEPATH = "../assets/lang-english.txt"
OUTPATH = "../assets/data.bin"

file_line_num = sum(1 for _ in open(FILEPATH))
buffer = bytes()

outfile = open(OUTPATH, "wb")
outfile.write(struct.pack("=L", file_line_num))

with open(FILEPATH) as infile:
    for index, line in enumerate(infile):
        line_bytes = bytes(line[:-1], "ascii")
        outfile.write(struct.pack("=L", len(line)))
        outfile.write(line_bytes)

outfile.close()
