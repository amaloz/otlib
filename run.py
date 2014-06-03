#!/usr/bin/env python2

from __future__ import print_function

import argparse

NITERS = 5

import time
import otlib.npot as npot

def sender(args):
    ot = npot.NPOTSender('127.0.0.1', 5000)
    start = time.time()
    for _ in xrange(NITERS):
        ot.send('hi', 'bye')
    end = time.time()
    print('Sender time (%d iterations): %f' % (NITERS, end - start))

def receiver(args):
    ot = npot.NPOTReceiver('127.0.0.1', 5000)
    start = time.time()
    for _ in xrange(NITERS):
        print(ot.receive(1))
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
