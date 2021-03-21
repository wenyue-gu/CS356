# -*- coding: utf-8 -*-
import json
import os
import time
import sys
import subprocess
import argparse


parser = argparse.ArgumentParser()
parser.add_argument('-t', metavar='test', type=int, nargs=1, default=None, help='test cases')
args = parser.parse_args()

path = '../router/sr'
if __name__ == '__main__':
    if not os.path.exists(path):
        print("File not exist: %s" % path)
    else:
        with open(os.devnull, 'w') as devnull:
            pox_proc = subprocess.Popen("./pox/pox.py cs144.ofhandler cs144.srhandler".split(), stdout=devnull, stderr=devnull)
            time.sleep(3)
            if args.t is None:
                mininet_proc = subprocess.Popen("sudo python2 ./testcases.py".split(), stdout=sys.stdout, stderr=sys.stdout)
            else:
                command = "sudo python2 ./testcases.py -t %d"  % args.t[0]
                mininet_proc = subprocess.Popen(command.split(), stdout=sys.stdout, stderr=sys.stdout)
            time.sleep(3)
            sr_proc = subprocess.Popen("%s" % path, stdout=devnull, stderr=devnull)
            mininet_proc.communicate()
            sr_proc.kill()
            pox_proc.kill()

            infile = open("lab4_results.json", "r")
            result = infile.read()
            infile.close()

            print(result)