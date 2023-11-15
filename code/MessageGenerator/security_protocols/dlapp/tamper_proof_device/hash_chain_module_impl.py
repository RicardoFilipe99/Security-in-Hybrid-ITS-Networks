import hashlib
import logging

from security_protocols.dlapp.models.hash_chain import HashChain
from common.util import create_signed_byte_array_from


class HashChainModuleImpl:
    """
    Prototype implementation of the Hash Chain Module Interface
    """

    def __init__(self, hash_algorithm):
        self.hash_algorithm = hash_algorithm

    def hash_chain_generation(self, seed, chain_size):
        """
        Method to calculate an hash chain

        :param seed: Chain's seed (bytes)
        :param chain_size: Number of hashes
        :return: HashChain object
        """

        logging.debug(f'hash_chain_generation called with seed={seed} and chain_size={chain_size}')

        hash_chain = []

        to_digest = seed

        for i in range(1, chain_size + 1):
            md = hashlib.new(self.hash_algorithm)
            md.update(to_digest)

            to_digest = md.digest()
            to_digest = create_signed_byte_array_from(byte_array=to_digest)

            hash_chain.append(to_digest)

        return HashChain(hash_chain)
