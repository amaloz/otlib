from __future__ import print_function

import _naorpinkas as _np

class NPOTSender(object):
    def __init__(self, host, port):
        self._state = _np.init(host, repr(port), True)

    def send(self, m0, m1):
        _np.send(self._state, repr(m0), repr(m1))

class NPOTReceiver(object):
    def __init__(self, host, port):
        self._state = _np.init(host, repr(port), False)

    def receive(self, choice):
        return _np.receive(self._state, choice)
