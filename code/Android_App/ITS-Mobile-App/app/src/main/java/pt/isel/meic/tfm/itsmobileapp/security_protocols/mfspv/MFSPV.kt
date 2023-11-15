package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv

import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import pt.isel.meic.tfm.itsmobileapp.security_protocols.Common
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.ITamperProofDeviceDLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.ITamperProofDeviceMFSPV

// Specific tag for debug
const val TAG_MFSPV = "ITSMobApp-MFSPV"

/**
 * A security protocol for ITS (extended for hybrid networks)
 *
 * Prototype implementation of:
 * A Multi-Factor Secured and Lightweight Privacy-Preserving Authentication Scheme for VANETs
 *
 * Article: https://ieeexplore.ieee.org/document/9154685
 */
class MFSPV(
    private val tamperProofDevice : ITamperProofDeviceMFSPV,
) : ISecurityProtocol {
    override val protocolDesignation : Common.ProtocolEnum = Common.ProtocolEnum.MFSPV

    /**
     * Protect message with MFSPV protocol
     */
    override fun applySecurityProtocolEncapsulation(itsMessage: ITSMessage): ITSMessage {
        return tamperProofDevice.applySecurityProtocolEncapsulation(itsMessage = itsMessage)
    }

    /**
     * Verify and deprotect message with MFSPV protocol
     */
    override fun applySecurityProtocolDecapsulation(itsMessage: ITSMessage): ITSMessage? {
        return tamperProofDevice.applySecurityProtocolDecapsulation(itsMessage = itsMessage)
    }

}