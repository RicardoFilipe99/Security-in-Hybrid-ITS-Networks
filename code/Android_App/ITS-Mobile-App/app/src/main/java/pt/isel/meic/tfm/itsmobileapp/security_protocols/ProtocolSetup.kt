package pt.isel.meic.tfm.itsmobileapp.security_protocols

import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.DLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.biometric_device.BiometricDeviceImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.biometric_device.IBiometricDevice
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.ITamperProofDeviceDLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.TamperProofDeviceDLAPPImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.hash_chain.HashChainModuleImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.pseudo_identity.PseudoIdentityModuleImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.MFSPV
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.ITamperProofDeviceMFSPV
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.TamperProofDeviceMFSPVImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.pseudo_identity.IPseudoIdentityModMFSPV
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.pseudo_identity.PseudoIdentityModMFSPVImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.no_security.NoSecurity

class ProtocolSetup {

    companion object {

        fun getSecurityProtocol(protocol: Common.ProtocolEnum): ISecurityProtocol {
            return when (protocol) {
                Common.ProtocolEnum.DLAPP -> dlappSetup()
                Common.ProtocolEnum.MFSPV -> mfspvSetup()
                Common.ProtocolEnum.NONE -> noSecuritySetup()
            }
        }

        /**
         * No security
         */
        private fun noSecuritySetup() : NoSecurity {
            return NoSecurity()
        }

        /**
         * DLAPP protocol configuration and initialisation
         */
        private fun dlappSetup() : DLAPP {

            // Define algorithm arguments
            val secretSystemKey : ByteArray = byteArrayOf(-17, -12, -59, -126, -37, -128, -5, 80, -30, -36, 27, -82, -113, 69, 60, 80, -64, 65, 99, -44, 99, 124, 105, -38, -95, 25, -7, 19, 38, 45, 38, 45)
            val hashChainElements : Int = 5
            val numberOfPseudoIdentities : Int = 3
            val hashAlgorithm : String = "SHA-256"
            val signatureAlgorithm : String = "HmacSHA256"

            // Create an instance of type ITamperProofDevice
            val tpd : ITamperProofDeviceDLAPP = TamperProofDeviceDLAPPImpl(
                hashChainModule = HashChainModuleImpl(
                    hashAlgorithm = hashAlgorithm
                ),
                pseudoIdentityModule = PseudoIdentityModuleImpl(),
                secretSystemKey = secretSystemKey,
                numberOfHashChainElements = hashChainElements,
                numberOfPseudoIdentities = numberOfPseudoIdentities,
                macAlgorithm = signatureAlgorithm,
            )

            // Create an instance of type IBiometricDevice
            val bd : IBiometricDevice = BiometricDeviceImpl()

            return DLAPP(
                biometricDevice = bd,
                tamperProofDevice = tpd
            )

        }

        /**
         * MFSPV protocol configuration and initialisation
         */
        private fun mfspvSetup() : MFSPV {
            val pidGenerator : IPseudoIdentityModMFSPV = PseudoIdentityModMFSPVImpl(
                hashAlgorithm = "SHA-256",
                apiNew = byteArrayOf(
                    -6, -6, 6, -1, -3, -1, -51, 81, -71, 1, -71, 1,
                    -6, -6, 6, -1, -3, -1, -51, 81, -71, 1,
                    -6, -6, 6, -1, -3, -1, -51, 81, -71, 1
                ),

                idV = byteArrayOf(
                    -10, -20, -30, -40, -50, -60, -70, -80, -90, -10, -90, -10,
                    -10, -20, -30, -40, -50, -60, -70, -80, -90, -10,
                    -10, -20, -30, -40, -50, -60, -70, -80, -90, -10
                ),

                vSk = byteArrayOf(
                    -60, -62, 60, -10, -37, -12, -5, 8, -7, 19, -7, 19,
                    -60, -62, 60, -10, -37, -12, -5, 8, -7, 19,
                    -60, -62, 60, -10, -37, -12, -5, 8, -7, 19
                ),

                kMbr = byteArrayOf(
                    -64, -65, 4, -40, -7, -2, 5, -8, -11, 22, -11, 22,
                    -64, -65, 4, -40, -7, -2, 5, -8, -11, 22,
                    -64, -65, 4, -40, -7, -2, 5, -8, -11, 22
                )
            )

            val tpd : ITamperProofDeviceMFSPV = TamperProofDeviceMFSPVImpl(
                pseudoIdentityModule = pidGenerator,
                // Region's Key (32 bytes)
                regionalKey = byteArrayOf(
                    -16, 9, -52, -51, -42, -96, 104, 101, 7, 17, -91, 42,
                    -72, 56, 112, 76, 103, 115, -19, -33, 31, -119,
                    82, 113, -94, 114, -112, -81, -57, -94, 106, -40),
                hashAlgorithm = "SHA-256"
            )

            return MFSPV(tamperProofDevice = tpd)
        }

    }

}