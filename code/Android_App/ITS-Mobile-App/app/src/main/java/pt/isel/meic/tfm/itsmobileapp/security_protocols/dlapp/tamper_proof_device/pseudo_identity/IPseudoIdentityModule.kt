package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.pseudo_identity

import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.PseudoIdentityDLAPP

/**
 * Interface definition for the Pseudo Identity Module
 *
 * It allows the creation of a Pseudo Identity used to allow user's privacy
 */
interface IPseudoIdentityModule {
    fun createPseudoIdentity(): PseudoIdentityDLAPP
}