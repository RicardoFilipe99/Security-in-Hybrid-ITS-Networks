package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device

import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.TAG_DLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.HashChain
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models.PseudoIdentityDLAPP
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.hash_chain.IHashChainModule
import pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.tamper_proof_device.pseudo_identity.IPseudoIdentityModule
import pt.isel.meic.tfm.itsmobileapp.common.ByteUtils
import pt.isel.meic.tfm.itsmobileapp.common.CryptographyUtils
import java.nio.ByteBuffer
import java.time.Instant
import kotlin.random.Random

/**
 * Prototype implementation of TamperProofDevice Interface
 */
class TamperProofDeviceDLAPPImpl(

    // Modules
    private val hashChainModule : IHashChainModule,
    private val pseudoIdentityModule : IPseudoIdentityModule,

    // Other
    private val secretSystemKey : ByteArray,
    private val numberOfHashChainElements : Int,
    private val numberOfPseudoIdentities : Int,
    private val macAlgorithm : String,

    ) : ITamperProofDeviceDLAPP {

    private val hashChainObj: HashChain
    private val pseudoIds: Array<PseudoIdentityDLAPP>

    /**
     * Fields' size
     */
    private val pseudoIdTotalBytes = 20
    private val macTotalBytes = 12
    private val randomKeyIndexTotalBytes = 4
    private val timestampTotalBytes = 4

    /**
     * Indexes to read
     *
     * The message are concatenated like this:
     * pseudoId + mac + randomKeyIndex + timestamp + itsMessage
     */

    // Read pseudo id
    private val readPseudoIdStart = 0
    private val readPseudoIdEnd = pseudoIdTotalBytes

    // Read mac
    private val readMacStart = readPseudoIdEnd
    private val readMacEnd = readMacStart + macTotalBytes

    // Read random key index
    private val readRandomKeyIdxStart = readMacEnd
    private val readRandomKeyIdxEnd = readRandomKeyIdxStart + randomKeyIndexTotalBytes

    // Read timestamp
    private val readTimestampStart = readRandomKeyIdxEnd
    private val readTimestampEnd = readTimestampStart + timestampTotalBytes

    // Read ITS message
    private val readItsMessageStart = readTimestampEnd

    init {
        Log.i(TAG_DLAPP,"TamperProofDevicePrototypeImpl being created with the following arguments:\n " +
                "caSecretKey / Hash chain seed : ${secretSystemKey.contentToString()}\n" +
                "numberOfHashChainElements : $numberOfHashChainElements\n" +
                "numberOfPseudoIdentities : $numberOfPseudoIdentities\n" +
                "signatureAlgorithm : $macAlgorithm")

        /**
         * Create an hash chain and pseudo identities
         */
        Log.d(TAG_DLAPP, "About to create an hash chain.")
        hashChainObj = hashChainModule.createHashChain(seed = secretSystemKey, size = numberOfHashChainElements)

        Log.d(TAG_DLAPP, "About to create pseudo identities.")
        pseudoIds = Array(numberOfPseudoIdentities) { pseudoIdentityModule.createPseudoIdentity() }

        Log.i(TAG_DLAPP, "hashChain : $hashChainObj\npseudoIds: ${pseudoIds.joinToString(",\n")}.")
    }

    /**
     * Protect message with DLAPP protocol
     */
    override fun applySecurityProtocolEncapsulation(itsMessage: ITSMessage): ITSMessage {
        Log.i(TAG_DLAPP, "DLAPP [applySecurityProtocolEncapsulation] called.")

        /*
         * ITS message (variable length)
         */
        Log.v(TAG_DLAPP, "ITS message: ${itsMessage.bytes.contentToString()}")
        Log.v(TAG_DLAPP, "ITS message size: ${itsMessage.bytes.size}")

        /*
         * Pseudo identity (20 bytes)
         */
        val pseudoId : PseudoIdentityDLAPP = pseudoIds.random()

        Log.v(TAG_DLAPP, "PID: ${pseudoId.bytes.contentToString()}")
        Log.v(TAG_DLAPP, "PID size: ${pseudoId.bytes.size}")

        /*
         * Random Key Index (4 bytes)
         */
        val randomKeyIndex: Int = Random.nextInt(numberOfHashChainElements) // [0, numberOfHashChainElements[

        val randomKeyIndexBytes: ByteArray =
            ByteBuffer.allocate(Integer.BYTES).putInt(randomKeyIndex).array()

        Log.v(TAG_DLAPP, "randomKeyIndex: $randomKeyIndex")
        Log.v(TAG_DLAPP, "randomKeyIndexBytes: ${randomKeyIndexBytes.contentToString()}")
        Log.v(TAG_DLAPP, "randomKeyIndexBytes size: ${randomKeyIndexBytes.size}")

        /*
         * Timestamp (4 bytes)
         */
        val timestamp =
            Instant.now().epochSecond.toInt()

        val timestampBytes = ByteBuffer.allocate(Integer.BYTES).putInt(timestamp).array()

        Log.v(TAG_DLAPP, "timestamp: $timestamp")
        Log.v(TAG_DLAPP, "timestampBytes: ${timestampBytes.contentToString()}")
        Log.v(TAG_DLAPP, "timestampBytes size: ${timestampBytes.size}")

        /*
         * Signature (12 bytes)
         */
        val contentToSign: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            pseudoId.bytes,     // PID
            itsMessage.bytes,   // m
            timestampBytes      // Ts
        )

        val mac: ByteArray = performMAC(
            hashKey = hashChainObj.hashChain[randomKeyIndex],
            fieldsToSign = contentToSign,
            algorithm = macAlgorithm
        )

        /*
         * Concatenation, total bytes = (20 + 12 + 4 + 4 + variable) bytes
         */
        val messageWithDlappProtocol: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            pseudoId.bytes,
            mac,
            randomKeyIndexBytes,
            timestampBytes,
            itsMessage.bytes
        )

        Log.v(TAG_DLAPP, "messageWithDlappProtocol: ${messageWithDlappProtocol.contentToString()}")
        Log.v(TAG_DLAPP, "messageWithDlappProtocol size: ${messageWithDlappProtocol.size}")

        Log.v(TAG_DLAPP, "DLAPP [applySecurityProtocol] returning.")
        return ITSMessage(messageWithDlappProtocol)

    }

    /**
     * Verify and deprotect message with DLAPP protocol
     */
    override fun applySecurityProtocolDecapsulation(itsMessage: ITSMessage): ITSMessage? {
        Log.d(TAG_DLAPP, "DLAPP [applySecurityProtocolDecapsulation] called.")

        try {

            /**
             * Check the timestamp's validity
             */
            val tsBytes = itsMessage.bytes.sliceArray(this.readTimestampStart until this.readTimestampEnd)
            val ts = ByteBuffer.wrap(tsBytes).int
            val currentTs = Instant.now().epochSecond.toInt()

            Log.v(TAG_DLAPP, "Received timestamp: $ts")
            Log.v(TAG_DLAPP, "System timestamp: $currentTs")

            if (ts == currentTs) {
                Log.v(TAG_DLAPP, "Timestamp validated!")
            } else {
                Log.v(TAG_DLAPP, "Timestamp not validated!")
                return null
            }

            /**
             * Verify signature
             *
             * First, it is necessary to extract the bytes from each field. The message was concatenated like this:
             * pseudoId + mac + randomKeyIndex + timestamp + itsMessage
             * The timestamp is not necessary to extract because it was already previously extracted.
             */

            // -----------------
            // 1. Extract fields
            // -----------------

            val pseudoIdBytes = itsMessage.bytes.sliceArray(
                readPseudoIdStart until readPseudoIdEnd)  // read 20 bytes

            val macBytes = itsMessage.bytes.sliceArray(
                readMacStart until readMacEnd)  // read 12 bytes

            val randomKeyIndexBytes = itsMessage.bytes.sliceArray(
                readRandomKeyIdxStart until readRandomKeyIdxEnd)  // read 4 bytes

            val itsMessageBytes = itsMessage.bytes.sliceArray(
                readItsMessageStart until itsMessage.bytes.size)

            // ---------------------------------
            // 2. Perform and compare signatures
            // ---------------------------------

            // Get key
            val idxValue = ByteBuffer.wrap(randomKeyIndexBytes).int
            Log.v(TAG_DLAPP, "idx_value $idxValue")
            Log.v(TAG_DLAPP, "key: ${hashChainObj.hashChain[idxValue]}")

            val msg = pseudoIdBytes + itsMessageBytes + tsBytes
            val calculatedMacSignature = performMAC(
                hashKey = hashChainObj.hashChain[idxValue],
                fieldsToSign = msg,
                algorithm = macAlgorithm
            )

            return if (calculatedMacSignature.contentEquals(macBytes)) {

                Log.i(TAG_DLAPP, "Message successfully verified! The calculated signature is equal to the received one, mac verified with success.")

                ITSMessage(itsMessageBytes)

            } else {
                Log.d(TAG_DLAPP, "The calculated signature is not equal to the received one, mac not verified.")

                null

            }
        } catch (e: Exception) {
            Log.w(TAG_DLAPP, "Exception thrown in verifyMessage, exception message: ${e.message}.\nReturning null.")
            return null
        }

    }

    private fun performMAC(
        hashKey: ByteArray,
        fieldsToSign: ByteArray,
        algorithm: String
    ) : ByteArray {

        val mac: ByteArray = CryptographyUtils.performDigitalSignature(hashKey, fieldsToSign, algorithm)

        Log.v(TAG_DLAPP, "MAC: ${mac.contentToString()}")
        Log.v(TAG_DLAPP, "MAC size: ${mac.size}")

        // According to the DLAPP protocol, the output signature of the MAC operation shall be truncated to only 12 bytes.
        // Although the actual length of the hashed values using HMAC-SHA-256 algorithm is 20 bytes
        val macLeastSignificant12bytes = ByteArray(12)
        System.arraycopy(mac, 20, macLeastSignificant12bytes, 0, 12)

        Log.v(TAG_DLAPP, "macLeastSignificant12bytes: ${macLeastSignificant12bytes.contentToString()}")
        Log.v(TAG_DLAPP, "macLeastSignificant12bytes size: ${macLeastSignificant12bytes.size}")

        return macLeastSignificant12bytes
    }

}