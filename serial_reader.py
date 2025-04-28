#!/usr/bin/env python3

import time
import serial
import datetime
import binascii

import termcolor
import subprocess


def format_delta(delta):
    tms = delta.total_seconds() * 1000
    if tms < 5000:
        return '%.0fms' % tms
    tmss = tms % 1000;
    tms /= 1000
    if tms <= 60:
        return '%.2fs%dms' % (tms, tmss)
    tms = int(tms)
    mins = tms // 60
    secs = tms % 60
    return '%dm%ds%dms' % (mins, secs, tmss)


def format_line(line, elffile):
    color = None
    if line.startswith('Backtrace: ') and elffile is not None:
        stacktrace = line[len('Backtrace: '):].strip().split(' ')
        proc = subprocess.Popen(['xtensa-esp32-elf-addr2line', '-fe', elffile, *stacktrace], stdout=subprocess.PIPE)
        proc.wait()
        line = proc.stdout.read().decode()
    if line.startswith('I '):
        color = 'blue'
    if line.startswith('W '):
        color = 'yellow'
    if line.startswith('E '):
        color = 'red'
    if color is not None:
        return termcolor.colored(line, color)
    return line


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('port')
    parser.add_argument('baud', default=115200, type=int)
    parser.add_argument('--elffile', required=False)
    args = parser.parse_args()
    start = datetime.datetime.now()
    while True:
        try:
            with serial.Serial(args.port, args.baud) as port:
                start = datetime.datetime.now()
                while True:
                    line = port.readline()
                    try:
                        line = line.decode().strip()
                        linetype = 'S'
                    except:
                        line = binascii.hexlify(line).decode()
                        linetype = 'B'
                    delta = (datetime.datetime.now() - start)
                    print('[%09s] [%02s] - %s' % ('+%s' % format_delta(delta), linetype, format_line(line, args.elffile)))
        except Exception as e:
            print('[%09s] [%02s] - %s' % ('READER', '', '%s' % (e,)))
            time.sleep(2)
