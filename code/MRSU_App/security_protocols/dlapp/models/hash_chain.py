class HashChain:
    """
    Class representing an hash chain
    """

    def __init__(self, hash_chain):
        self.chain = hash_chain

    def __str__(self):
        representation = "HashChain(hashChain="
        representation += "\n"
        for hash_bytes in self.chain:
            representation += str(hash_bytes) + "\n"
        representation += ")"

        return representation
