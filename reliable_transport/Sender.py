import sys
import argparse
import Reliable

parser = argparse.ArgumentParser()
parser.add_argument("-p", metavar='local_port', type=int, default=10001, help="port for the local end (default 10001)")
parser.add_argument("-r", metavar='remote_port', type=int, default=50001,
                    help="port for the remote end (default 50001)")
parser.add_argument('filename', metavar='filename', type=str, nargs=1, help='file to be transferred')
args = parser.parse_args()

reli = Reliable.Reliable(args.p, args.r)
reli.connect()

filename = sys.argv[1]
fin = open(args.filename[0], "rb")
while True:
    block = fin.read(Reliable.BlockSize)  # we use BlockSize here for simplicity
    if not block:
        break
    reli.send(block)
fin.close()
reli.close()
