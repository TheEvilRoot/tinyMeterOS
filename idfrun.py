#!/usr/bin/env python3

import os
import enum
import signal
import subprocess

class Command(enum.Enum):
    FLASH = 'flash'
    RELOAD = 'reload'
    BUILD = 'build'
    CLEAN = 'clean'
    QUIT = 'quit'
    MENU = 'menu'

class SerialMonitor:
    def __init__(self, port, elffile):
        self.port = port
        self.proc = None
        self.elffile = elffile

    def stop(self):
        if self.proc is not None:
            self.proc.send_signal(signal.SIGTERM)

    def start(self):
        if self.proc is not None:
            self.proc.kill()
        self.proc = subprocess.Popen('python3 serial_reader.py '+ self.port + ' 115200 --elffile ' + self.elffile, shell=True)

class IDF:
    def __init__(self, port, args):
        self.port = port
        self.args = args

    def run(self, target):
        subprocess.call(['idf.py', *target], env={**os.environ, 'ESPPORT': self.port})

def run_idf(idf, monitor, target):
    stopped = False
    if idf.port == monitor.port:
        monitor.stop()
        stopped = True
    idf.run(target)
    if stopped:
        monitor.start()

def read_command():
    import getch
    try:
        char = getch.getch()
        if char == 'r': return Command.FLASH
        if char == 'R': return Command.RELOAD
        if char == 'b': return Command.BUILD
        if char == 'c': return Command.CLEAN
        if char == 'q': return Command.QUIT
        if char == 'm': return Command.MENU
    except:
        return None
    return None

def run_console(idf, monitor):
    while True:
        try:
            command = read_command()
            if command == Command.FLASH:
                run_idf(idf, monitor, ['flash'])
            elif command == Command.QUIT:
                monitor.stop()
            elif command == Command.BUILD:
                run_idf(idf, monitor, ['build'])
            elif command == Command.CLEAN:
                run_idf(idf, monitor, ['clean'])
            elif command == Command.RELOAD:
                monitor.stop()
                monitor.start()
            elif command == Command.MENU:
                run_idf(idf, monitor, ['menuconfig'])
        except KeyboardInterrupt:
            print('Interrupt, stopping monitor')
            monitor.stop()

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('port')
    args = parser.parse_args()
    idf = IDF(args.port, [])
    monitor = SerialMonitor(args.port, f'build/tinyMeterOS.elf')
    monitor.start()
    try:
        run_console(idf, monitor)
    finally:
        monitor.stop()

if __name__ == '__main__':
    main()


