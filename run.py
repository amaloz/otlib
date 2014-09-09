#!/usr/bin/env python2

from __future__ import print_function

import argparse, random

N = 2
MAXLENGTH = 4

import time
import otlib.ot_np as np
import otlib.ot_pvw as pvw
import otlib.otext_iknp as iknp
import otlib.otext_nnob as nnob
import otlib._otlib as _ot

def sender(args):
    state = _ot.init('127.0.0.1', repr(5000), 80, True)
    msgs = (('a' * MAXLENGTH, 'b' * MAXLENGTH),) * args.niters
    start = time.time()
    if args.test_iknp:
        ot = iknp.OTExtSender(state)
        ot.send(msgs, MAXLENGTH, np)
    if args.test_nnob:
        ot = nnob.OTExtSender(state)
        ot.send(msgs, pvw)
    if args.test_np:
        ot = np.OTSender(state)
        ot.send(msgs, MAXLENGTH)
    if args.test_pvw:
        ot = pvw.OTSender(state)
        ot.send(msgs, MAXLENGTH)
    end = time.time()
    print('Sender time (%d iterations): %f' % (args.niters, end - start))
        

def receiver(args):
    state = _ot.init('127.0.0.1', repr(5000), 80, False)
    choices = [random.randint(0, 1) for _ in xrange(args.niters)]
    start = time.time()
    if args.test_iknp:
        ot = iknp.OTExtReceiver(state)
        r = ot.receive(choices, MAXLENGTH, np)
    if args.test_nnob:
        ot = nnob.OTExtReceiver(state)
        r = ot.receive(choices, pvw)
    if args.test_np:
        ot = np.OTReceiver(state)
        r = ot.receive(choices, MAXLENGTH)
    if args.test_pvw:
        ot = pvw.OTReceiver(state)
        r = ot.receive(choices, MAXLENGTH)
    end = time.time()
    print(r[:10])
    print('Receiver time (%d iterations): %f' % (args.niters, end - start))


def main():
    parser = argparse.ArgumentParser(
        description='Run oblivious transfer.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    subparsers = parser.add_subparsers()

    parser_sender = subparsers.add_parser(
        'sender',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='commands for OT sender')
    parser_sender.add_argument(
        '--test-iknp', action='store_true',
        help='test IKNP semi-honest OT extension implementation')
    parser_sender.add_argument(
        '--test-nnob', action='store_true',
        help='test NNOB malicious OT extension implementation')
    parser_sender.add_argument(
        '--test-np', action='store_true',
        help='test Naor-Pinkas semi-honest OT implementation')
    parser_sender.add_argument(
        '--test-pvw', action='store_true',
        help='test PVW malicious OT implementation')
    parser_sender.add_argument(
        '--niters', action='store', type=int, default=80,
        help='number of iterations')
    parser_sender.set_defaults(func=sender)

    parser_receiver = subparsers.add_parser(
        'receiver',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='commands for OT receiver')
    parser_receiver.add_argument(
        '--test-iknp', action='store_true',
        help='test IKNP semi-honest OT extension implementation')
    parser_receiver.add_argument(
        '--test-nnob', action='store_true',
        help='test NNOB malicious OT extension implementation')
    parser_receiver.add_argument(
        '--test-np', action='store_true',
        help='test Naor-Pinkas semi-honest OT implementation')
    parser_receiver.add_argument(
        '--test-pvw', action='store_true',
        help='test PVW malicious OT implementation')
    parser_receiver.add_argument(
        '--niters', action='store', type=int, default=80,
        help='number of iterations')
    parser_receiver.set_defaults(func=receiver)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
