#!/usr/bin/env python3

from opster import command, dispatch
from collections import defaultdict
import datetime
import gzip

def parse_time(s):
    s = s.split('.')
    milliseconds = int(s[1])
    s = s[0].split(':')
    hours   = int(s[0])
    minutes = int(s[1])
    seconds = int(s[2])

    return ((hours + 60*minutes)*60 + seconds)*1000000 + milliseconds

def format_time(t):
    milliseconds = t % 1000000
    seconds      = t // 1000000
    minutes      = seconds // 60
    seconds     %= 60
    hours        = minutes // 60
    minutes     %= 60
    return "%02d:%02d:%02d.%06d" % (hours, minutes, seconds, milliseconds)

def is_gzipfile(fn):
    with open(fn, 'rb') as f:
        return f.read(2) == b'\x1f\x8b'

def events(fn):
    open_ = open

    if is_gzipfile(fn):
        open_ = gzip.open

    with open_(fn, 'rt') as f:
        for line in f:
            line  = line.split()
            rk    = int(line[0])
            time  = parse_time(line[1])
            begin = True if line[2][0] == '<' else False
            name  = line[2][1:]
            yield rk, time, begin, name

@command()
def show(fn, segment,
          total = ('t', False, "show total only")):
    """Count the number of occurrences and total time of a segment in the profile"""

    count = defaultdict(int)
    count_total = 0
    time  = defaultdict(int)
    time_total = 0
    for rk, t, begin, name in events(fn):
        if name == segment:
            if begin:
                count_total += 1
                count[rk] += 1
                last = t
            else:
                time[rk] += t - last
                time_total += t - last

    if not total:
        for rk in sorted(count.keys()):
            print(str(rk).rjust(10), str(count[rk]).rjust(20), format_time(time[rk]).rjust(20))

    print("Total:".rjust(10), str(count_total).rjust(20), format_time(time_total).rjust(20))

if __name__ == '__main__':
    dispatch()
