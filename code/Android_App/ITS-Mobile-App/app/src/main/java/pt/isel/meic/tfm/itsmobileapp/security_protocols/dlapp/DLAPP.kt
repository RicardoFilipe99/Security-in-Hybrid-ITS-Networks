package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp

import android.security.keystore.UserNotAuthenticatedException
import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import pt.isel.meic.tfm.itsmobileapp.security_protocols.Common
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.biometric_device.IBiometricDevice
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.ITamperProofDeviceDLAPP

// Specific tag for debug
const val TAG_DLAPP = "ITSMobApp-DLAPP"

/**
 * A security protocol for ITS (extended for hybrid networks)
 *
 * Prototype implementation of:
 * A Decentralized Lightweight Authentication and Privacy Protocol for Vehicular Networks
 *
 * Article: https://ieeexplore.ieee.org/document/8811470
 */
class DLAPP(
    private val biometricDevice : IBiometricDevice,
    private val tamperProofDevice : ITamperProofDeviceDLAPP,
) : ISecurityProtocol {

    override val protocolDesignation : Common.ProtocolEnum = Common.ProtocolEnum.DLAPP

    init {
        Log.i(TAG_DLAPP,"DLAPP instance creation.")

        // Biometric device authentication
        if (!biometricDevice.authenticateUser()) {
            Log.w(TAG_DLAPP, "User not successfully authenticated.")
            throw UserNotAuthenticatedException("User was not successfully authenticated by the Biometric Device")
        }

        Log.i(TAG_DLAPP, "User successfully authenticated.")
    }

    /**
     * Protect message with DLAPP protocol
     */
    override fun applySecurityProtocolEncapsulation(itsMessage : ITSMessage): ITSMessage {
        return tamperProofDevice.applySecurityProtocolEncapsulation(itsMessage)
    }

    /**
     * Verify and deprotect message with DLAPP protocol
     */
    override fun applySecurityProtocolDecapsulation(itsMessage : ITSMessage): ITSMessage? {
        return tamperProofDevice.applySecurityProtocolDecapsulation(itsMessage)
    }

}