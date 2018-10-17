#!/usr/bin/env python3

from opster import command, dispatch
from collections import defaultdict
import datetime
import gzip
import re

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

    stack = []
    with open_(fn, 'rt') as f:
        for line in f:
            line  = line.split()
            rk    = int(line[0])
            time  = parse_time(line[1])
            begin = True if line[2][0] == '<' else False
            name  = line[2][1:]
            if begin:
                stack.append(name)
                full_name = ':'.join(stack)
            else:
                full_name = ':'.join(stack)
                stack.pop()
            yield rk, time, begin, name, full_name
    if len(stack) != 0:
        print("Warning: stack not empty at the end of the profile")

@command()
def show(fn, segment='',
          regex      = ('r', '',    "regular expression to apply"),
          full_name  = ('f', False, "use full names"),
          stats_only = ('', False, "show stats only"),
          ranks_only = ('', False, "don't show stats")):
    """Show the number of occurrences and total time spent in different segments of the profile"""

    count       = defaultdict(lambda: defaultdict(int))
    time        = defaultdict(lambda: defaultdict(int))

    if regex:
        regex_prog = re.compile(regex)

    last = {}
    for rk, t, begin, name, full_name_ in events(fn):
        last_name_ = name
        if full_name:
            name = full_name_
        if not name: continue
        if (not segment and not regex) or name == segment or (regex and regex_prog.match(name)):
            if begin:
                count[name][rk] += 1
                last[name]       = t
            else:
                time[name][rk]  += t - last[name]

    for name in sorted(count.keys()):
        print(name)
        if not stats_only:
            for rk in sorted(count[name].keys()):
                print(str(rk).rjust(10), str(count[name][rk]).rjust(20), format_time(time[name][rk]).rjust(20))
        if not ranks_only:
            max_count = max(count[name].values())
            min_count = min(count[name].values())
            sum_count = sum(count[name].values())
            max_time  = max(time[name].values())
            min_time  = min(time[name].values())
            sum_time  = sum(time[name].values())
            print("  Count:")
            print("    min: ", str(min_count).rjust(21))
            print("    max: ", str(max_count).rjust(21))
            print("    sum: ", str(sum_count).rjust(21))
            print("  Time:")
            print("    min: ", format_time(min_time).rjust(42))
            print("    max: ", format_time(max_time).rjust(42))
            print("    sum: ", format_time(sum_time).rjust(42))

if __name__ == '__main__':
    dispatch()
