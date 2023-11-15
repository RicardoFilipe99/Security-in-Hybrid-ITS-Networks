package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.pseudo_identity

import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.PseudoIdentityDLAPP
import pt.isel.meic.tfm.itsmobileapp.common.ByteUtils
import pt.isel.meic.tfm.itsmobileapp.common.generateByteArray
import java.security.MessageDigest
import kotlin.random.Random

/**
 * Prototype implementation of the Pseudo Identity Module Interface
 */
class PseudoIdentityModuleImpl : IPseudoIdentityModule {

    /**
     * Function that generates pseudo-identities
     *
     * pidOne (dynamic part) gives a hashing value of a random number 'randomLong' generated for each pseudo-identity.
     *
     * pidTwo ('static' part) is calculated by a XOR of the initial pseudo-identity of each ITS-S and the concatenation of and ca system key
     *
     * The generated pseudo-identity is the result of the concatenation between pidOne and pidTwo
     *
     * According to DLAPP protocol, to provide anonymity, each TPD generates a group of pseudo-identities
     * with each identity composed of two parts PID1 and PID2. These pseudo-identities can hide
     * the real identity of the ITS-S from other ITS stations and prevent the tracking attacks.
     *
     * The structure of the pseudo-identity is defined in a way that allows only the CA to retrieve the real identity at any time.
     */
    override fun createPseudoIdentity(): PseudoIdentityDLAPP { // Initial version, still not using pidOne and pidTwo (to be implemented)

        fun pidOne(
            hashAlgorithm : String
        ) : ByteArray {
            val randomLong = Random.nextLong(0, Long.MAX_VALUE) // SecureRandom?
            val md = MessageDigest.getInstance(hashAlgorithm)

            return md.digest(ByteUtils.longToBytes(randomLong))
        }

        fun pidTwo(
            pidInit : ByteArray,
            pidOne : ByteArray,
            hashAlgorithm : String,
            caSystemKey : ByteArray
        ) : ByteArray {
            val md = MessageDigest.getInstance(hashAlgorithm)
            return ByteArray(1)
        }

        val pseudoIdTotalBytes = 20
        return PseudoIdentityDLAPP(generateByteArray(pseudoIdTotalBytes))
    }

}

