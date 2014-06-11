#!/usr/bin/env python2

from __future__ import print_function

import argparse

NITERS = 160
N = 2
MAXLENGTH = 4

import time
import otlib.npot as npot
import otlib.otext as otext
import otlib._otlib as _ot

def sender(args):
    start = time.time()
    state = _ot.init('127.0.0.1', repr(5000), 80, True)
    ot = otext.OTExtSender(state)
    # ot = npot.NPOTSender(state)
    msgs = (('hi', 'bye'),) * NITERS
    ot.send(msgs)
    end = time.time()
    print('Sender time (%d iterations): %f' % (NITERS, end - start))

def receiver(args):
    start = time.time()
    state = _ot.init('127.0.0.1', repr(5000), 80, False)
    ot = otext.OTExtReceiver(state)
    # ot = npot.NPOTReceiver(state)
    choices = (1,) * NITERS
    print(ot.receive(choices))
    end = time.time()
    print('Receiver time (%d iterations): %f' % (NITERS, end - start))

def main():
    parser = argparse.ArgumentParser(
        description='Run oblivious transfer.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    subparsers = parser.add_subparsers()

    parser_sender = subparsers.add_parser(
        'sender',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='commands for OT sender')
    parser_sender.set_defaults(func=sender)

    parser_receiver = subparsers.add_parser(
        'receiver',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='commands for OT receiver')
    parser_receiver.set_defaults(func=receiver)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
