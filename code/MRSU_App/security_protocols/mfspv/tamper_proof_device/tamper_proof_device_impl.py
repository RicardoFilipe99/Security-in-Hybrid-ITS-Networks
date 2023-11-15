import hashlib
import logging
import struct
import time

from common.util import create_signed_byte_array_from


class TamperProofDeviceMFSPVImpl:
    """
    Prototype implementation of TamperProofDevice Interface
    """

    def __init__(self, pseudo_identity_module, regional_key, hash_algorithm):
        self.hash_algorithm = hash_algorithm
        self.pseudo_identity_module = pseudo_identity_module
        self.regional_key = regional_key
        logging.debug(f'TamperProofDeviceImpl being initialized.')

        # ------------------------------------------ #
        # ------ Fields size (40 bytes total) ------ #
        # ------------------------------------------ #
        self.pseudo_id_total_bytes = 20
        self.mac_total_bytes = 20
        self.timestamp_total_bytes = 4

        # ----------------------------------------- #
        # ------------ Indexes to read ------------ #
        # ----------------------------------------- #

        # The message was concatenated like this:
        # timestamp + mac + pseudoId + itsMessage

        # Read timestamp
        self.read_timestamp_start = 0
        self.read_timestamp_end = self.timestamp_total_bytes

        # Read mac
        self.read_mac_start = self.read_timestamp_end
        self.read_mac_end = self.read_mac_start + self.mac_total_bytes

        # Read pseudo id
        self.read_pseudo_id_start = self.read_mac_end
        self.read_pseudo_id_end = self.read_pseudo_id_start + self.pseudo_id_total_bytes

        # Read ITS message
        self.read_its_message_start = self.read_pseudo_id_end

    def apply_security_protocol_encapsulation(self, its_message):
        """
        Protection function
        @param its_message: ITS Message (e.g., CAM)
        @return: Protected ITS Message
        """
        logging.debug(f'apply_security_protocol_encapsulation called. Message bytes: {its_message}')
        its_message = create_signed_byte_array_from(its_message)

        logging.debug(f"its_messageBytes: {list(its_message)}")
        logging.debug(f"its_messageBytes size: {len(its_message)}")

        # ϕvi = h(PID || Rk || m || t)

        """
        Timestamp (4 bytes)
        """
        timestamp = int(time.time())
        timestamp_bytes = create_signed_byte_array_from(byte_array=timestamp.to_bytes(4, 'big'))

        logging.debug(f"timestamp: {timestamp}")
        logging.debug(f"timestampBytes: {list(timestamp_bytes)}")
        logging.debug(f"timestampBytes size: {len(timestamp_bytes)}")

        """
        PID (20 bytes)
        """
        # Call PID generator
        pid = self.pseudo_identity_module.generate_pseudo_identity(t=timestamp_bytes)
        pseudo_id_bytes = pid.byte_array

        logging.debug(f"pid: {pid}")
        logging.debug(f"pseudo_id_bytes: {list(pseudo_id_bytes)}")
        logging.debug(f"pseudo_id_bytes size: {len(pseudo_id_bytes)}")

        """
        MAC (20 bytes)
        """
        mac_content = pseudo_id_bytes + self.regional_key + its_message + timestamp_bytes

        logging.debug(f"mac_content to calculate mac; {mac_content}")
        logging.debug(f"mac_content to calculate mac signed; {create_signed_byte_array_from(mac_content)}")

        md = hashlib.new(self.hash_algorithm)
        md.update(mac_content)
        mac = md.hexdigest()

        mac_bytes = create_signed_byte_array_from(byte_array=bytes.fromhex(mac))

        # Extract the last 20 bytes
        mac_bytes = mac_bytes[-20:]

        logging.debug(f"MAC: {list(mac_bytes)}")
        logging.debug(f"MAC size: {len(mac_bytes)}")

        """
        Concatenation, total bytes = (4 + 20 + 20 + variable) bytes
        """
        # Broadcasts the beacon: {t, ϕvi, PIDv, m}

        message_with_mfspv_protocol = timestamp_bytes + mac_bytes + pseudo_id_bytes + its_message
        logging.debug(f"message_with_mfspv_protocol: {list(message_with_mfspv_protocol)}")
        logging.debug(f"message_with_mfspv_protocol size: {len(message_with_mfspv_protocol)}")

        logging.debug("returning from apply_security_protocol_encapsulation.")

        return bytes(message_with_mfspv_protocol)

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
            timestamp + mac + pseudoId + itsMessage

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

            # ITS Message
            its_message_bytes = encapsulated_its_message[self.read_its_message_start:]

            logging.debug(f"its message bytes: {create_signed_byte_array_from(its_message_bytes)}")
            logging.debug(f"its message bytes size: {len(its_message_bytes)}")

            # ---------------------------
            # 2. Perform and compare macs
            # ---------------------------

            """
            MAC (20 bytes)
            """
            msg = pseudo_id_bytes + self.regional_key + its_message_bytes + ts_bytes
            logging.debug(f"msg to calculate mac; {msg}")
            logging.debug(f"msg to calculate mac signed; {create_signed_byte_array_from(msg)}")

            md = hashlib.new(self.hash_algorithm)
            md.update(msg)
            calculated_mac = md.hexdigest()

            calculated_mac_bytes = create_signed_byte_array_from(byte_array=bytes.fromhex(calculated_mac))

            logging.debug(f"Calculated MAC: {calculated_mac_bytes}")

            # Extract the last 20 bytes
            calculated_mac_signature_bytes = calculated_mac_bytes[-20:]

            logging.debug(f"Calculated MAC: {create_signed_byte_array_from(calculated_mac_signature_bytes)}")
            logging.debug(f"Received MAC: {mac_bytes}")

            if calculated_mac_signature_bytes == mac_bytes:
                logging.info("Message successfully verified with MFSPV protocol.")
                return its_message_bytes
            else:
                logging.warning("The calculated mac is not equal to the received one, mac not verified.")
                return None

        except Exception as e:
            logging.warning(f"Exception thrown in MFSPV protocol decapsulation, "
                            f"exception message: ${repr(e)}. Returning None.")
            return None

    def get_security_fields_bytes(self, encapsulated_its_message):
        logging.debug(f'get_security_fields_bytes called. Message: {encapsulated_its_message}')

        try:
            # pseudo-identity PIDv, q , hash signature ϕvi, and time stamp of sizes 20, 20, and 4 bytes

            pseudo_id_bytes = encapsulated_its_message[self.read_pseudo_id_start: self.read_pseudo_id_end]

            mac_bytes = encapsulated_its_message[self.read_mac_start: self.read_mac_end]

            ts_bytes = encapsulated_its_message[self.read_timestamp_start: self.read_timestamp_end]

            security_fields_bytes = ts_bytes + mac_bytes + pseudo_id_bytes

            return create_signed_byte_array_from(byte_array=security_fields_bytes)

        except Exception as e:
            logging.warning(f"Exception thrown in MFSPV protocol. Exception message: ${repr(e)}.")
            raise Exception("Exception thrown in MFSPV protocol.")