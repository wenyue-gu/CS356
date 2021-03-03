import os
import sys
import time
import json
import argparse
import subprocess
import signal

# inpath = "/autograder/submission"
# outpath = "/autograder/results"

parser = argparse.ArgumentParser()
parser.add_argument("-c", action='store_true', help="use C code")
parser.add_argument('-t', metavar='test', type=int, nargs=1, default=None, help='test cases')
parser.add_argument('inpath', metavar='inpath', type=str, nargs='?', default='./', help='path to the code directory')
parser.add_argument('outpath', metavar='outpath', type=str, nargs='?',
                    default='./', help='path to the result directory')
args = parser.parse_args()
if args.c:
    print("Running C code ...")
else:
    print("Running Python code ...")

os.chdir(args.inpath)


def killprocs():
    os.system("pkill --signal 9 -f Sender >/dev/null 2>&1")
    os.system("pkill --signal 9 -f Receiver >/dev/null 2>&1")
    time.sleep(1)


def killed(signum, frame):
    killprocs()
    exit(0)


signal.signal(signal.SIGTERM, killed)
signal.signal(signal.SIGINT, killed)


def run(param):
    try:
        generator = subprocess.Popen("./generator 12582912 a.txt".split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
        generator.wait()
        cmd = "./Receiver -p 50001 -s %d -d %d -f %d temp.txt" % (param["synloss"], param["dataloss"], param["finloss"])
        receiver = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
        if args.c:
            cmd = "./Sender a.txt -p 10000 -r 50001"
        else:
            cmd = "python3 ./Sender.py a.txt -p 10000 -r 50001"
        if "seqnum" in params:
            cmd += ' -n %d' % params["seqnum"]
        sender = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
    except Exception as e:
        print(e)
        return 0

    try:
        start = time.time()
        out, err = receiver.communicate(timeout=param["timeout"])
        out, err = sender.communicate(timeout=param["timeout"])
        end = time.time()
        print("Finished in %.2f seconds" % (end-start))
    except Exception as e:
        receiver.kill()
        sender.kill()
        out, err = receiver.communicate()
        if Exception is subprocess.TimeoutExpired:
            print("Timed out after %d seconds" % param["timeout"])
        else:
            print(e)
        return 0

    res = 1
    with subprocess.Popen(("diff -q ./a.txt ./temp.txt").split(), stdout=subprocess.PIPE, stderr=sys.stderr) as proc:
        out, err = proc.communicate()
        if len(out) != 0:
            print("Corrupted file received")
            res = 0
    return res


killprocs()

params = [
    {"dataloss": 0, "synloss": 0, "finloss": 0, "timeout": 35, 'seqnum': 0},
    {"dataloss": 0, "synloss": 0, "finloss": 0, "timeout": 35, 'seqnum': 4294967296},
    {"dataloss": 1, "synloss": 0, "finloss": 0, "timeout": 50},
    {"dataloss": 1, "synloss": 0, "finloss": 0, "timeout": 50, 'seqnum': 4294957296},
    {"dataloss": 5, "synloss": 0, "finloss": 0, "timeout": 60},
    {"dataloss": 5, "synloss": 0, "finloss": 0, "timeout": 60, 'seqnum': 4294864896},
    {"dataloss": 1, "synloss": 50, "finloss": 25, "timeout": 75},
    {"dataloss": 1, "synloss": 50, "finloss": 25, "timeout": 75, 'seqnum': 4293508096},
    {"dataloss": 5, "synloss": 50, "finloss": 25, "timeout": 90},
    {"dataloss": 5, "synloss": 50, "finloss": 25, "timeout": 90, 'seqnum': 4291637248}
]


def runTestCase(i):
    param = params[i]
    case = {
        "score": 0,
        "max_score": 10,
        "visibility": "visible",
        "output": json.dumps(param)
    }

    print("Case %d: " % (i+1), param)
    sc = run(param)*10
    case["score"] += sc
    print("Points: %d/10\n" % sc)
    return case


if args.t is not None:
    runTestCase(args.t[0]-1)
    exit(0)

cases = []
score = 0
for i in range(0, 10):
    case = runTestCase(i)
    cases.append(case)
    score += case["score"]

print("Total: %d/100" % score)

res = {
    "score": score,
    "stdout_visibility": "visible",
    "tests": cases
}

with open('%s/results.json' % args.outpath, 'w') as fout:
    fout.write(json.dumps(res))
