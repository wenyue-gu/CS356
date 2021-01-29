import os
import sys
import time
import json
import subprocess

# inpath = "/autograder/submission"
# outpath = "/autograder/results"
inpath = "./"
outpath = "./"

if len(sys.argv) > 2:
    inpath = sys.argv[1]
    outpath = sys.argv[2]


def run(param):
    generator = subprocess.Popen("./generator 12582912 a.txt".split(), stdout=subprocess.DEVNULL)
    generator.wait()

    cmd = "%s/Receiver -p 50001 -s %d -d %d -f %d temp.txt" % (inpath,
                                                               param["synloss"], param["dataloss"], param["finloss"])
    receiver = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL)
    cmd = "python3 %s/Sender.py a.txt -p 10000 -r 50001" % inpath
    sender = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL)
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
        out, err = sender.communicate()
        if Exception is subprocess.TimeoutExpired:
            print("Timed out after %d seconds" % param["timeout"])
        return 0

    res = 1
    with subprocess.Popen(("diff -q %s/a.txt %s/temp.txt" % (inpath, inpath)).split(), stdout=subprocess.PIPE) as proc:
        out, err = proc.communicate()
        if len(out.decode()) != 0:
            res = 0
    return res


params = [{"dataloss": 0, "synloss": 0, "finloss": 0, "timeout": 30},
          {"dataloss": 1, "synloss": 0, "finloss": 0, "timeout": 45},
          {"dataloss": 5, "synloss": 0, "finloss": 0, "timeout": 60},
          {"dataloss": 1, "synloss": 50, "finloss": 25, "timeout": 45},
          {"dataloss": 5, "synloss": 50, "finloss": 25, "timeout": 65}]

cases = []
score = 0
i = 0
for param in params:
    for j in range(0, 2):
        i += 1
        case = {
            "score": 0,
            "max_score": 10,
            "visibility": "visible",
            "output": json.dumps(param)
        }

        print("Case %d: " % i, param)
        sc = run(param)*10
        case["score"] += sc
        cases.append(case)
        score += sc
        print("Points: %d/10\n" % sc)

print("Total: %d/100" % score)

res = {
    "score": score,
    "stdout_visibility": "visible",
    "tests": cases
}

with open('%s/results.json' % outpath, 'w') as fout:
    fout.write(json.dumps(res))
