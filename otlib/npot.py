from __future__ import print_function

import _naorpinkas as _np

class NPOTSender(object):
    def __init__(self, host, port):
        self._state = _np.init(host, repr(port), True)

    def send(self, msgs):
        _np.send(self._state, msgs)

class NPOTReceiver(object):
    def __init__(self, host, port):
        self._state = _np.init(host, repr(port), False)

    def receive(self, N, choices):
        return _np.receive(self._state, N, choices)
