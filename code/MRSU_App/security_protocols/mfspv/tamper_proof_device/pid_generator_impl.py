import hashlib

from common.util import generate_random_byte_array, create_signed_byte_array_from, xor_byte_arrays
from security_protocols.mfspv.models.pseudo_identity import PseudoIdentityMFSPV


class PseudoIdentityMFSPVImpl:
    """
    Implementation of the Pseudo Identity Module Interface
    """

    def __init__(self, hash_algorithm, api_new, v_sk, id_v, k_mbr):
        self.hash_algorithm = hash_algorithm

        self.api_new = api_new
        self.v_sk = v_sk
        self.id_v = id_v
        self.k_mbr = k_mbr

        # PIDv = h(APInew || Vsk || IDv || Kmbr) ⊕ h(APInew || t)
        # static_part = h(APInew || Vsk || IDv || Kmbr)
        md = hashlib.new(self.hash_algorithm)
        md.update(self.api_new + self.v_sk + self.id_v + self.k_mbr)
        self.static_part = md.digest()

    def generate_pseudo_identity(self, t):  # Prototype version

        # PIDv = h(APInew || Vsk || IDv || Kmbr) ⊕ h(APInew || t)
        # dynamic_part = h(APInew || t)
        to_digest = self.api_new + t

        md = hashlib.new(self.hash_algorithm)
        md.update(to_digest)
        dynamic_part = md.digest()

        xor_static_dynamic = xor_byte_arrays(self.static_part, dynamic_part)

        # Trunc to last 20 bytes
        pid_bytes = xor_static_dynamic[-20:]

        return PseudoIdentityMFSPV(
            create_signed_byte_array_from(byte_array=pid_bytes))
