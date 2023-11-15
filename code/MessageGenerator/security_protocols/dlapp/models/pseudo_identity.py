class PseudoIdentityDLAPP:
    """
    Class representing a PID (Pseudo-Identity)

    Pseudo-identities can hide the real identity of the ITS-S from other ITS stations and prevent tracking attacks
    """

    def __init__(self, byte_array):
        self.byte_array = byte_array

    def __eq__(self, other):
        """
        Check if two PseudoIdentity objects are equal based on their byte_array
        """
        if isinstance(other, PseudoIdentityDLAPP):
            return self.byte_array == other.byte_array
        return False

    def __ne__(self, other):
        """
        Check if two PseudoIdentity objects are not equal based on their byte_array
        """
        return not self.__eq__(other)

    def __str__(self):
        return f"PseudoIdentity(bytes={self.byte_array})"
