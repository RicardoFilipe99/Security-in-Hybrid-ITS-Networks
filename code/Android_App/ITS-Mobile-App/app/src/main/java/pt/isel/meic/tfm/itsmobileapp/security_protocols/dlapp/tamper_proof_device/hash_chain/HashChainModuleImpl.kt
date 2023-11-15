package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.hash_chain

import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.TAG_DLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.HashChain
import java.security.MessageDigest

/**
 * Prototype implementation of the Hash Chain Module Interface
 */
class HashChainModuleImpl(
    private val hashAlgorithm: String
) : IHashChainModule {

    override fun createHashChain(seed: ByteArray, size: Int): HashChain {

        val md = MessageDigest.getInstance(hashAlgorithm)
        val hashChain = Array<ByteArray>(size) { byteArrayOf() }

        var toDigest : ByteArray = seed

        for (i in 1..size) {
            toDigest = md.digest(toDigest)
            hashChain[i - 1] = toDigest

            //Log.v(TAG_DLAPP, "Hash idx $i: ${toDigest.contentToString()}")

            /*//Log.d(TAG_DLAPP, "Hash idx $i (hex): " + toDigest.joinToString(separator = "") {
                String.format("%02X", it)
            })*/
        }

        return HashChain(hashChain)
    }

}