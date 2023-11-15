import logging

from security_protocols.protocol_setup import SecurityProtocolEnum


class NoSecurity:
    """
    Only returns what it receives (no security)
    """

    def __init__(self):
        logging.info(f'Initialising NoSecurity protocol.')

        self.protocol_designation = SecurityProtocolEnum.NONE

    def apply_security_protocol_encapsulation(self, its_message):
        logging.debug(f'Security not being used, so encapsulation is not needed.')
        return its_message

    def apply_security_protocol_decapsulation(self, encapsulated_its_message):
        logging.debug(f'Security not being used, so decapsulation is not needed.')
        return encapsulated_its_message

    def get_security_fields_bytes(self, encapsulated_its_message):
        logging.debug(f'Security not being used, so there are not security bytes.')
        return None
