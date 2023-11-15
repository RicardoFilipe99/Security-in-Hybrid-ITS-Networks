import logging

from security_protocols.protocol_setup import SecurityProtocolEnum


class MFSPV:
    """
    A security protocol for ITS (extended for hybrid networks)

    Prototype implementation of:
    A Multi-Factor Secured and Lightweight Privacy-Preserving Authentication Scheme for VANETs

    Article: https://ieeexplore.ieee.org/document/9154685
    """

    def __init__(self, tamper_proof_device):
        logging.info(f'Initialising MFSPV protocol.')

        self.tpd = tamper_proof_device
        self.protocol_designation = SecurityProtocolEnum.MFSPV

    def apply_security_protocol_encapsulation(self, its_message):
        """
        Protects message with MFSPV protocol
        @param its_message: original message
        @return: protected message (in MFSPV format)
        """
        return self.tpd.apply_security_protocol_encapsulation(its_message=its_message)

    def apply_security_protocol_decapsulation(self, encapsulated_its_message):
        """
        Verifies and deprotects message with MFSPV protocol
        @param encapsulated_its_message: protected message (in MFSPV format)
        @return: original message
        """
        return self.tpd.apply_security_protocol_decapsulation(encapsulated_its_message=encapsulated_its_message)

    def get_security_fields_bytes(self, encapsulated_its_message):
        """
        Gets MFSPV-specific security bytes from protected message
        @param encapsulated_its_message: protected message (in MFSPV format)
        @return: security bytes
        """
        return self.tpd.get_security_fields_bytes(encapsulated_its_message=encapsulated_its_message)
