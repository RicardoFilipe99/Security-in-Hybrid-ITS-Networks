package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.pseudo_identity

import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.models.PseudoIdentityMFSPV

/**
 * Interface definition for the Pseudo Identity Module
 *
 * It allows the creation of a Pseudo Identity used to allow user's privacy
 */
interface IPseudoIdentityModMFSPV {
    fun createPseudoIdentity(timestamp : Int): PseudoIdentityMFSPV
}