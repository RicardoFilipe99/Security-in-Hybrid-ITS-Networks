package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device

import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage

/**
 * Interface definition with the Tamper Proof Device
 *
 * TPD - Hardware security module device to store the ITS-S's pseudonyms and the secret keys.
 * The protocol assumes that TPDs make it extremely difficult to compromise the cryptographic
 * material kept inside the hardware. Responsibility: Secure key storage and processing.
 *
 */
interface ITamperProofDeviceMFSPV {
    /**
     * Protect message with MFSPV protocol
     */
    fun applySecurityProtocolEncapsulation(itsMessage: ITSMessage) : ITSMessage

    /**
     * Verify and deprotect message with MFSPV protocol
     */
    fun applySecurityProtocolDecapsulation(itsMessage: ITSMessage) : ITSMessage?
}