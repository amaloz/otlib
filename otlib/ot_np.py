import _otlib as _ot
import ot

class OTSender(ot.OTSender):
    def __init__(self, state):
        super(OTSender, self).__init__(state, _ot.ot_np_send)

class OTReceiver(ot.OTReceiver):
    def __init__(self, state):
        super(OTReceiver, self).__init__(state, _ot.ot_np_receive)
