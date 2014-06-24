#!/usr/bin/env python2

from __future__ import print_function

import argparse, random

N = 2
MAXLENGTH = 4

import time
import otlib.ot_np as np
import otlib.otext as otext
import otlib.ot_pvw as pvw
import otlib._otlib as _ot

def sender(args):
    if not args.test_np and not args.test_otext and not args.test_pvw:
        print('one of --test-np, --test-otext, --test-pvw must be used')
        return
    state = _ot.init('127.0.0.1', repr(5000), 80, True)
    msgs = (('a' * MAXLENGTH, 'b' * MAXLENGTH),) * args.niters
    start = time.time()
    if args.test_otext:
        ot = otext.OTExtSender(state)
        ot.send(msgs, MAXLENGTH, np)
    if args.test_np:
        ot = np.OTSender(state)
        ot.send(msgs, MAXLENGTH)
    if args.test_pvw:
        ot = pvw.OTSender(state)
        ot.send(msgs, MAXLENGTH)
    end = time.time()
    print('Sender time (%d iterations): %f' % (args.niters, end - start))
        

def receiver(args):
    if not args.test_np and not args.test_otext and not args.test_pvw:
        print('one of --test-np, --test-otext, --test-pvw must be used')
        return
    state = _ot.init('127.0.0.1', repr(5000), 80, False)
    choices = [random.randint(0, 1) for _ in xrange(args.niters)]
    start = time.time()
    if args.test_otext:
        ot = otext.OTExtReceiver(state)
        r = ot.receive(choices, MAXLENGTH, np)
    if args.test_np:
        ot = np.OTReceiver(state)
        r = ot.receive(choices, MAXLENGTH)
    if args.test_pvw:
        ot = pvw.OTReceiver(state)
        r = ot.receive(choices, MAXLENGTH)
    end = time.time()
    print(r)
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
        '--test-np', action='store_true',
        help='test Naor-Pinkas OT implementation')
    parser_sender.add_argument(
        '--test-otext', action='store_true',
        help='test OT extension implementation')
    parser_sender.add_argument(
        '--test-pvw', action='store_true',
        help='test PVW OT implementation')
    parser_sender.add_argument(
        '--niters', action='store', type=int, default=80,
        help='number of iterations')
    parser_sender.set_defaults(func=sender)

    parser_receiver = subparsers.add_parser(
        'receiver',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='commands for OT receiver')
    parser_receiver.add_argument(
        '--test-np', action='store_true',
        help='test Naor-Pinkas OT implementation')
    parser_receiver.add_argument(
        '--test-otext', action='store_true',
        help='test OT extension implementation')
    parser_receiver.add_argument(
        '--test-pvw', action='store_true',
        help='test PVW OT implementation')
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
