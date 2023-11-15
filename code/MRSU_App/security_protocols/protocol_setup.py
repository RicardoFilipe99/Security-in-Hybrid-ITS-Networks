import logging

from security_protocols.common import SecurityProtocolEnum
from security_protocols.dlapp.dlapp_impl import DLAPP
from security_protocols.dlapp.biometric_device.biometric_device_impl import BiometricDeviceImpl
from security_protocols.dlapp.tamper_proof_device.pseudo_identity_module_impl import PseudoIdentityModuleImpl
from security_protocols.dlapp.tamper_proof_device.hash_chain_module_impl import HashChainModuleImpl
from security_protocols.dlapp.tamper_proof_device.tamper_proof_device_impl import TamperProofDeviceDLAPPImpl
from common.util import create_signed_byte_array_from
from security_protocols.mfspv.mfspv_impl import MFSPV
from security_protocols.mfspv.tamper_proof_device.pid_generator_impl import PseudoIdentityMFSPVImpl
from security_protocols.mfspv.tamper_proof_device.tamper_proof_device_impl import TamperProofDeviceMFSPVImpl
from security_protocols.no_security.no_security_impl import NoSecurity


class ProtocolSetup:

    @staticmethod
    def get_security_protocol(security_prot):
        logging.debug(f'get_security_protocol called with security_prot = {security_prot}')

        if security_prot == SecurityProtocolEnum.NONE:
            return ProtocolSetup.no_security_setup()

        elif security_prot == SecurityProtocolEnum.MFSPV:
            return ProtocolSetup.mfspv_setup()

        elif security_prot == SecurityProtocolEnum.DLAPP:
            return ProtocolSetup.dlapp_setup()

    @staticmethod
    def no_security_setup():
        """
        NoSecurity configuration
        @return: no security object
        """
        logging.info(f'No security setup')

        return NoSecurity()

    @staticmethod
    def dlapp_setup():
        """
        DLAPP configuration
        @return: security protocol object with DLAPP configuration
        """
        logging.info(f'DLAPP protocol setup')

        # ---------------------------- #
        # ------ dlapp Protocol ------ #
        # ---------------------------- #

        secret_system_key = create_signed_byte_array_from(byte_array=[-17, -12, -59, -126, -37, -128, -5, 80,
                                                                      -30, -36, 27, -82, -113, 69, 60, 80, -64,
                                                                      65, 99, -44, 99, 124, 105, -38, -95, 25,
                                                                      -7, 19, 38, 45, 38, 45])

        number_of_hash_chain_elements = 5
        number_of_pseudo_identities = 3
        hash_algorith = "sha256"

        pseudo_id_module = PseudoIdentityModuleImpl()
        hash_chain_module = HashChainModuleImpl(hash_algorithm=hash_algorith)

        tp_device = TamperProofDeviceDLAPPImpl(
            hash_chain_module=hash_chain_module,
            pseudo_identity_module=pseudo_id_module,
            secret_system_key=secret_system_key,
            number_of_hash_chain_elements=number_of_hash_chain_elements,
            number_of_pseudo_identities=number_of_pseudo_identities
        )
        biometric_device = BiometricDeviceImpl(return_value=True)

        return DLAPP(
            biometric_device=biometric_device,
            tamper_proof_device=tp_device)

    @staticmethod
    def mfspv_setup():
        """
        MFSPV configuration
        @return: security protocol object with MFSPV configuration
        """

        pid_module = PseudoIdentityMFSPVImpl(
            hash_algorithm="sha256",

            # ITS-S specific info:
            api_new=create_signed_byte_array_from(byte_array=[
                -17, -19, -9, -120, -37, -12, -5, 80, -7, 19, -7, 19,
                -17, -19, -9, -120, -37, -12, -5, 80, -7, 19,
                -17, -19, -9, -120, -37, -12, -5, 80, -7, 19
            ]),

            v_sk=create_signed_byte_array_from(byte_array=[
                -11, -12, -13, -14, -15, -16, -17, -18, -19, -20, -19, -20,
                -11, -12, -13, -14, -15, -16, -17, -18, -19, -20,
                -11, -12, -13, -14, -15, -16, -17, -18, -19, -20
            ]),

            id_v=create_signed_byte_array_from(byte_array=[
                -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -9, -10,
                -1, -2, -3, -4, -5, -6, -7, -8, -9, -10,
                -1, -2, -3, -4, -5, -6, -7, -8, -9, -10
            ]),

            k_mbr=create_signed_byte_array_from(byte_array=[
                -11, -19, -9, -120, -37, -12, -5, 80, -7, 19, -7, 19,
                -11, -19, -9, -120, -37, -12, -5, 80, -7, 19,
                -11, -19, -9, -120, -37, -12, -5, 80, -7, 19
            ])

        )

        tp_device = TamperProofDeviceMFSPVImpl(
            pseudo_identity_module=pid_module,
            # Region's Key (32 bytes)
            regional_key=create_signed_byte_array_from(byte_array=[
                -16, 9, -52, -51, -42, -96, 104, 101, 7, 17, -91, 42,
                -72, 56, 112, 76, 103, 115, -19, -33, 31, -119,
                82, 113, -94, 114, -112, -81, -57, -94, 106, -40
            ]),
            hash_algorithm="sha256"
        )

        return MFSPV(tamper_proof_device=tp_device)
