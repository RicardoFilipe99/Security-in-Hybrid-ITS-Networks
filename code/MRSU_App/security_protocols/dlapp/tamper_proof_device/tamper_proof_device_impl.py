import hashlib
import hmac
import logging
import random
import struct
import time

from common.util import create_signed_byte_array_from


class TamperProofDeviceDLAPPImpl:
    """
    Prototype implementation of TamperProofDevice Interface
    """

    def __init__(self, hash_chain_module, pseudo_identity_module,
                 secret_system_key, number_of_hash_chain_elements, number_of_pseudo_identities):

        logging.debug(f'TamperProofDeviceImpl being initialized.')

        # Modules
        self.hash_chain_module = hash_chain_module
        self.pseudo_identity_module = pseudo_identity_module

        # Others vars
        self.secret_system_key = secret_system_key
        self.number_of_pseudo_identities = number_of_pseudo_identities
        self.hash_chain_size = number_of_hash_chain_elements

        # ----------------------------------------- #
        # --------- Hash Chain Generation --------- #
        # ----------------------------------------- #
        logging.debug(f'About to call hash_chain_generation.')
        self.hash_chain = hash_chain_module.hash_chain_generation(seed=self.secret_system_key,
                                                                  chain_size=number_of_hash_chain_elements)
        logging.debug(f'Generated hash chain: {self.hash_chain}')

        # ---------------------------------------- #
        # ----- Pseudo Identities Generation ----- #
        # ---------------------------------------- #
        logging.debug(f'About to call generate_pseudo_identity.')
        self.pseudo_identities = []

        for i in range(number_of_hash_chain_elements):
            pid = pseudo_identity_module.generate_pseudo_identity()
            self.pseudo_identities.append(pid)

        logging.debug(f'Generated pseudo identities: {[str(pid) for pid in self.pseudo_identities]}')

        # ------------------------------------------ #
        # ------ Fields size (40 bytes total) ------ #
        # ------------------------------------------ #
        self.pseudo_id_total_bytes = 20
        self.mac_total_bytes = 12
        self.random_key_index_total_bytes = 4
        self.timestamp_total_bytes = 4

        # ----------------------------------------- #
        # ------------ Indexes to read ------------ #
        # ----------------------------------------- #

        # The message was concatenated like this:
        # pseudoId + mac + randomKeyIndex + timestamp + itsMessage

        # Read pseudo id
        self.read_pseudo_id_start = 0
        self.read_pseudo_id_end = self.pseudo_id_total_bytes

        # Read mac
        self.read_mac_start = self.read_pseudo_id_end
        self.read_mac_end = self.read_mac_start + self.mac_total_bytes

        # Read random key index
        self.read_random_key_idx_start = self.read_mac_end
        self.read_random_key_idx_end = self.read_random_key_idx_start + self.random_key_index_total_bytes

        # Read timestamp
        self.read_timestamp_start = self.read_random_key_idx_end
        self.read_timestamp_end = self.read_timestamp_start + self.timestamp_total_bytes

        # Read ITS message
        self.read_its_message_start = self.read_timestamp_end

    def apply_security_protocol_encapsulation(self, its_message):
        """
        Protection function
        @param its_message: ITS Message (e.g., CAM)
        @return: Protected ITS Message
        """
        logging.debug(f'apply_security_protocol_encapsulation called. Message bytes: {its_message}')
        its_message = create_signed_byte_array_from(its_message)

        """
        Pseudo identity (20 bytes)
        """
        pseudo_id = random.choice(self.pseudo_identities)
        pseudo_id_bytes = pseudo_id.byte_array
        logging.debug(f"PID: {pseudo_id}")

        """
        Random Key Index (4 bytes)
        """
        random_key_index = random.randint(0, self.hash_chain_size - 1)
        random_key_index_bytes = random_key_index.to_bytes(4, 'big')

        logging.debug(f"randomKeyIndex: {random_key_index}")
        logging.debug(f"randomKeyIndexBytes: {list(random_key_index_bytes)}")
        logging.debug(f"randomKeyIndexBytes size: {len(random_key_index_bytes)}")

        """
        Timestamp (4 bytes)
        """
        timestamp = int(time.time())
        timestamp_bytes = create_signed_byte_array_from(byte_array=timestamp.to_bytes(4, 'big'))

        logging.debug(f"timestamp: {timestamp}")
        logging.debug(f"timestampBytes: {list(timestamp_bytes)}")
        logging.debug(f"timestampBytes size: {len(timestamp_bytes)}")

        """
        Signature (12 bytes)
        """
        content_to_sign = pseudo_id_bytes + its_message + timestamp_bytes

        mac = hmac.new(
            key=self.hash_chain.chain[random_key_index].tobytes(),
            msg=content_to_sign,
            digestmod=hashlib.sha256
        ).hexdigest()

        mac_bytes = create_signed_byte_array_from(byte_array=bytes.fromhex(mac))

        logging.debug(f"MAC: {list(mac)}")
        logging.debug(f"MAC size: {len(mac)}")

        # Extract the last 12 bytes
        mac_bytes = mac_bytes[-12:]

        """
        Concatenation, total bytes = (20 + 12 + 4 + 4 + variable) bytes
        """
        message_with_dlapp_protocol = pseudo_id_bytes + mac_bytes + random_key_index_bytes + timestamp_bytes + its_message

        logging.debug(f"messageWithDlappProtocol: {list(message_with_dlapp_protocol)}")
        logging.debug(f"messageWithDlappProtocol size: {len(message_with_dlapp_protocol)}")

        logging.debug("returning from apply_security_protocol_encapsulation.")
        return message_with_dlapp_protocol

    def apply_security_protocol_decapsulation(self, encapsulated_its_message):
        """
        Deprotection function
        @param encapsulated_its_message: Protected ITS Message (e.g., CAM)
        @return: ITS Message
        """

        logging.debug(f'apply_security_protocol_decapsulation called. Message: {encapsulated_its_message}')

        try:
            # ------------------------------------------- #
            # ----- Checks the timestamp's validity ----- #
            # ------------------------------------------- #

            ts_bytes = encapsulated_its_message[self.read_timestamp_start: self.read_timestamp_end]
            ts = struct.unpack('>i', ts_bytes)[0]
            current_ts = int(time.time())

            logging.debug(f"Received timestamp: {ts}")
            logging.debug(f"System timestamp: {current_ts}")

            # Due to sync difficulties, we have the following condition:
            if ts == current_ts or (2 > ts - current_ts > -2):
                logging.debug("Timestamp validated!")
            else:
                logging.warning("Timestamp not validated!")
                return None

            # ---------------------------- #
            # ----- Verify signature ----- #
            # ---------------------------- #

            '''
            First, is necessary to extract the bytes from each field. The message was concatenated like this:
            pseudoId + mac + randomKeyIndex + timestamp + itsMessage

            The timestamp is not necessary to extract because it was already previously extracted.
            '''

            # -----------------
            # 1. Extract fields
            # -----------------

            # Pseudo id
            pseudo_id_bytes = encapsulated_its_message[
                              self.read_pseudo_id_start: self.read_pseudo_id_end]  # read 20 bytes

            logging.debug(f"pseudo_id_bytes: {create_signed_byte_array_from(pseudo_id_bytes)}")
            logging.debug(f"pseudo_id_bytes size: {len(pseudo_id_bytes)}")

            # MAC
            mac_bytes = encapsulated_its_message[self.read_mac_start: self.read_mac_end]  # read 12 bytes
            mac_bytes = create_signed_byte_array_from(byte_array=mac_bytes)

            logging.debug(f"mac bytes: {mac_bytes}")
            logging.debug(f"mac bytes size: {len(mac_bytes)}")

            # Random Key Index
            random_key_index_bytes = encapsulated_its_message[
                                     self.read_random_key_idx_start: self.read_random_key_idx_end]  # read 4 bytes

            logging.debug(f"random key index bytes: {create_signed_byte_array_from(random_key_index_bytes)}")
            logging.debug(f"random key index bytes size: {len(random_key_index_bytes)}")

            # ITS Message
            its_message_bytes = encapsulated_its_message[self.read_its_message_start:]

            logging.debug(
                f"its message bytes: {create_signed_byte_array_from(its_message_bytes)}")
            logging.debug(f"its message bytes size: {len(its_message_bytes)}")

            # ---------------------------
            # 2. Perform and compare macs
            # ---------------------------

            # Get key
            idx_value = int.from_bytes(random_key_index_bytes, byteorder='big')
            logging.debug(f"idx_value {idx_value}")
            logging.debug(f"key: {self.hash_chain.chain[idx_value]}")

            msg = pseudo_id_bytes + its_message_bytes + ts_bytes
            logging.debug(f"msg to calculate mac; {msg}")
            logging.debug(f"msg to calculate mac signed; {create_signed_byte_array_from(msg)}")

            calculated_mac_signature = hmac.new(
                key=self.hash_chain.chain[idx_value].tobytes(),
                msg=msg,
                digestmod=hashlib.sha256
            ).hexdigest()

            calculated_mac_signature_bytes = create_signed_byte_array_from(
                byte_array=bytes.fromhex(calculated_mac_signature))

            logging.debug(
                f"Calculated MAC: {create_signed_byte_array_from(calculated_mac_signature_bytes)}")
            logging.debug(f"Received MAC: {mac_bytes}")

            # Extract the last 12 bytes
            calculated_mac_signature_bytes = calculated_mac_signature_bytes[-12:]

            if calculated_mac_signature_bytes == mac_bytes:
                logging.info(
                    "Message successfully verified with DLAPP protocol.")
                return its_message_bytes
            else:
                logging.warning("The calculated mac is not equal to the received one, mac not verified.")
                return None

        except Exception as e:
            logging.warning(f"Exception thrown in DLAPP protocol decapsulation, "
                            f"exception message: ${repr(e)}. Returning None.")
            return None

    def get_security_fields_bytes(self, encapsulated_its_message):
        logging.debug(f'get_security_fields_bytes called. Message: {encapsulated_its_message}')

        try:

            pseudo_id_bytes = encapsulated_its_message[self.read_pseudo_id_start: self.read_pseudo_id_end]

            mac_bytes = encapsulated_its_message[self.read_mac_start: self.read_mac_end]

            random_key_index_bytes = encapsulated_its_message[
                                     self.read_random_key_idx_start: self.read_random_key_idx_end]

            ts_bytes = encapsulated_its_message[self.read_timestamp_start: self.read_timestamp_end]

            security_fields_bytes = pseudo_id_bytes + mac_bytes + random_key_index_bytes + ts_bytes

            return create_signed_byte_array_from(byte_array=security_fields_bytes)

        except Exception as e:
            logging.warning(f"Exception thrown in DLAPP protocol. Exception message: ${repr(e)}.")
            raise Exception("Exception thrown in DLAPP protocol.")
