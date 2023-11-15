package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.pseudo_identity

import pt.isel.meic.tfm.itsmobileapp.common.ByteUtils
import pt.isel.meic.tfm.itsmobileapp.common.ByteUtils.intToBytes
import pt.isel.meic.tfm.itsmobileapp.common.xorByteArrays
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.models.PseudoIdentityMFSPV
import java.security.MessageDigest

/**
 * Prototype implementation of the Pseudo Identity Module Interface
 * Pseudo Identities are used for user's privacy
 */
class PseudoIdentityModMFSPVImpl(
    private val hashAlgorithm : String,

    private val apiNew  : ByteArray,
    private val vSk  : ByteArray,
    private val idV : ByteArray,
    private val kMbr : ByteArray,

    ) : IPseudoIdentityModMFSPV {

    private lateinit var staticPart : ByteArray

    init {
        // PIDv = h(APInew || Vsk || IDv || Kmbr) ⊕ h(APInew || t)
        // static_part = h(APInew || Vsk || IDv || Kmbr)

        val md = MessageDigest.getInstance(hashAlgorithm)

        val contentToHash: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            apiNew,
            vSk,
            idV,
            kMbr,
        )

        staticPart = md.digest(contentToHash)
    }

    override fun createPseudoIdentity(timestamp : Int): PseudoIdentityMFSPV {
        // PIDv = h(APInew || Vsk || IDv || Kmbr) ⊕ h(APInew || t)
        // dynamic_part = h(APInew || t)
        val md = MessageDigest.getInstance(hashAlgorithm)

        val contentToHash: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            apiNew,
            intToBytes(timestamp)
        )


        val dynamicPart : ByteArray = md.digest(contentToHash)

        val xorStaticDynamic = xorByteArrays(staticPart, dynamicPart)

        val pidLeastSignificant20bytes = ByteArray(20)

        // SHA 256 has 32 bytes, we only want the last 20 (from 12 to 32)
        System.arraycopy(
            xorStaticDynamic, 12,
            pidLeastSignificant20bytes, 0, 20)

        return PseudoIdentityMFSPV(pidLeastSignificant20bytes)
    }

}