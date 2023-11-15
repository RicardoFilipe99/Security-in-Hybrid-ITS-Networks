import logging

from security_protocols.common import SecurityProtocolEnum


class DLAPP:
    """
    A security protocol for ITS (extended for hybrid networks)

    Prototype implementation of:
    A Decentralized Lightweight Authentication and Privacy Protocol for Vehicular Networks

    Article: https://ieeexplore.ieee.org/document/8811470
    """

    def __init__(self, biometric_device, tamper_proof_device):
        logging.info(f'Initialising DLAPP protocol.')

        self.protocol_designation = SecurityProtocolEnum.DLAPP

        # Biometric device authentication
        if not biometric_device.authenticate_user:
            logging.warning(f'User not successfully authenticated."')
            raise Exception("UserNotAuthenticated - User was not successfully authenticated by the Biometric Device")

        self.biometric_device = biometric_device
        self.tamper_proof_device = tamper_proof_device

        logging.info(f'User successfully authenticated.')

    def apply_security_protocol_encapsulation(self, its_message):
        """
        Protects message with DLAPP protocol
        @param its_message: original message
        @return: protected message (in DLAPP format)
        """
        return self.tamper_proof_device.apply_security_protocol_encapsulation(its_message=its_message)

    def apply_security_protocol_decapsulation(self, encapsulated_its_message):
        """
        Verifies and deprotects message with DLAPP protocol
        @param encapsulated_its_message: protected message (in DLAPP format)
        @return: original message
        """
        return self.tamper_proof_device.apply_security_protocol_decapsulation(
            encapsulated_its_message=encapsulated_its_message)

    def get_security_fields_bytes(self, encapsulated_its_message):
        """
        Gets DLAPP-specific security bytes from protected message
        @param encapsulated_its_message: protected message (in DLAPP format)
        @return: security bytes
        """
        return self.tamper_proof_device.get_security_fields_bytes(encapsulated_its_message=encapsulated_its_message)
