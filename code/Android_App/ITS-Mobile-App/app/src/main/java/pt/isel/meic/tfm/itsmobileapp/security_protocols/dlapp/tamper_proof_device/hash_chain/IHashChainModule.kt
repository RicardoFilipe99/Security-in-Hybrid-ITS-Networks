package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.hash_chain

import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.HashChain

/**
 * Interface definition for the Hash Chain Module
 *
 * It allows the creation of an Hash Chain to be used in the message authentication
 */
interface IHashChainModule {
    /**
     * By applying an hash function iteratively, starting with a seed, is produced a set of keys.
     * Each element of the chain is used later as a signing key for message authentication.
     * All legitimate users that belong to the ITS network will have the same set of keys, because
     * they all have the secret system key (seed), Ks, being able to sign and verify other messages.
     */
    fun createHashChain(seed: ByteArray, size : Int): HashChain
}