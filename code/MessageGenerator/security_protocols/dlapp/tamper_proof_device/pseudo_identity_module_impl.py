import logging
import random
import hashlib

from security_protocols.dlapp.models.pseudo_identity import PseudoIdentityDLAPP
from common.util import generate_random_byte_array


class PseudoIdentityModuleImpl:
    """
    Prototype implementation of the Pseudo Identity Module Interface
    """

    def __init__(self):
        pass

    def generate_pseudo_identity(self):  # Initial version, still not using pidOne and pidTwo (to be implemented)
        """
        Function that generates pseudo-identities using two helper functions:
        -   pidOne (dynamic part) gives a hashing value of a random number 'randomLong' generated for each pseudo-identity.
        -   pidTwo ('static' part) is calculated by a XOR of the initial pseudo-identity of each vehicle and the
        concatenation of and ca system key

        The generated pseudo-identity is the result of the concatenation between pidOne and pidTwo

        According to dlapp protocol, to provide anonymity, each TPD generates a group of pseudo-identities with each
        identity composed of two parts PID1 and PID2. These pseudo-identities can hide the real identity of the vehicle
        from other vehicles and prevent the tracking attacks.

        The structure of the pseudo-identity is defined in a way that allows only the CA to retrieve the real identity at
        any time.

        :return: PseudoIdentity object
        """

        logging.debug(f'generate_pseudo_identity called.')

        # Implementation not completed (using a prototype version).

        def pid_one(hash_algorithm):
            rand_number = random.randint(0, 2147483647)  # Generate random long value
            md = hashlib.new(hash_algorithm)
            rand_number_bytes = rand_number.to_bytes(4, 'big')

            return md.digest()

        def pid_two(pid_init, pid_one, hash_algorithm, ca_system_key):
            md = hashlib.new(hash_algorithm)
            hashValue = b""
            return bytes([0])  # Placeholder for pidTwo

        pseudo_id_total_bytes = 20
        return PseudoIdentityDLAPP(generate_random_byte_array(pseudo_id_total_bytes))
