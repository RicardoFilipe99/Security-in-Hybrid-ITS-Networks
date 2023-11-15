package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device

import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.common.ByteUtils
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.TAG_MFSPV
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.models.PseudoIdentityMFSPV
import pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.tamper_proof_device.pseudo_identity.IPseudoIdentityModMFSPV
import java.nio.ByteBuffer
import java.security.MessageDigest
import java.time.Instant


/**
 * Prototype implementation of TamperProofDevice Interface
 */
class TamperProofDeviceMFSPVImpl(
    // Modules
    private val pseudoIdentityModule : IPseudoIdentityModMFSPV,

    // Other
    private val regionalKey : ByteArray,
    private val hashAlgorithm : String,

) : ITamperProofDeviceMFSPV {

    /**
     * Fields' size
     */
    private val pseudoIdTotalBytes = 20
    private val macTotalBytes = 20
    private val timestampTotalBytes = 4

    /**
     * Indexes to read
     *
     * The message are concatenated like this:
     * timestamp + mac + pseudoId + itsMessage
     */

    // Read timestamp
    private val readTimestampStart = 0
    private val readTimestampEnd = readTimestampStart + timestampTotalBytes

    // Read mac
    private val readMacStart = readTimestampEnd
    private val readMacEnd = readMacStart + macTotalBytes

    // Read pseudo id
    private val readPseudoIdStart = readMacEnd
    private val readPseudoIdEnd = readPseudoIdStart + pseudoIdTotalBytes

    // Read ITS message
    private val readItsMessageStart = readPseudoIdEnd

    /**
     * Protect message with MFSPV protocol
     */
    override fun applySecurityProtocolEncapsulation(itsMessage: ITSMessage): ITSMessage {
        Log.i(TAG_MFSPV, "MFSPV [applySecurityProtocolEncapsulation] called.")
        // ϕvi = h(PID || Rk || m || t)

        /*
         * ITS message (variable length)
         */
        Log.v(TAG_MFSPV, "ITS message: ${itsMessage.bytes.contentToString()}")
        Log.v(TAG_MFSPV, "ITS message size: ${itsMessage.bytes.size}")

        /*
         * Timestamp (4 bytes)
         */
        val timestamp =
            Instant.now().epochSecond.toInt()

        val timestampBytes = ByteBuffer.allocate(Integer.BYTES).putInt(timestamp).array()

        Log.v(TAG_MFSPV, "timestamp: $timestamp")
        Log.v(TAG_MFSPV, "timestampBytes: ${timestampBytes.contentToString()}")
        Log.v(TAG_MFSPV, "timestampBytes size: ${timestampBytes.size}")

        /*
         * Pseudo identity (20 bytes)
         */
        val pseudoId : PseudoIdentityMFSPV = pseudoIdentityModule.createPseudoIdentity(timestamp = timestamp)

        Log.v(TAG_MFSPV, "PID: ${pseudoId.bytes.contentToString()}")
        Log.v(TAG_MFSPV, "PID size: ${pseudoId.bytes.size}")

        /*
         * MAC (20 bytes)
         */
        val contentToHash: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            pseudoId.bytes,     // PID
            regionalKey,        // rk
            itsMessage.bytes,   // m
            timestampBytes      // Ts
        )

        val md = MessageDigest.getInstance(hashAlgorithm)
        val mac : ByteArray = md.digest(contentToHash)

        val macLeastSignificant20bytes = ByteArray(20)

        // SHA 256 has 32 bytes, we only want the last 20 (from 12 to 32)
        System.arraycopy(
            mac, 12,
            macLeastSignificant20bytes, 0, 20)

        /*
         * Concatenation, total bytes = (20 + 12 + 4 + 4 + variable) bytes
         */
        val messageWithMfspvProtocol: ByteArray = ByteUtils.concatenateByteArrays( // vararg
            timestampBytes,
            macLeastSignificant20bytes,
            pseudoId.bytes,
            itsMessage.bytes,
        )

        Log.v(TAG_MFSPV, "messageWithMFSPVProtocol: ${messageWithMfspvProtocol.contentToString()}")
        Log.v(TAG_MFSPV, "messageWithMFSPVProtocol size: ${messageWithMfspvProtocol.size}")
        Log.v(TAG_MFSPV, "MFSPV [applySecurityProtocol] returning.")

        return ITSMessage(messageWithMfspvProtocol)
    }

    /**
     * Verify and deprotect message with MFSPV protocol
     */
    override fun applySecurityProtocolDecapsulation(itsMessage: ITSMessage): ITSMessage? {
        Log.d(TAG_MFSPV, "MFSPV [applySecurityProtocolDecapsulation] called.")

        try {

            /**
             * Check the timestamp's validity
             */
            val tsBytes = itsMessage.bytes.sliceArray(this.readTimestampStart until this.readTimestampEnd)
            val ts = ByteBuffer.wrap(tsBytes).int
            val currentTs = Instant.now().epochSecond.toInt()

            Log.v(TAG_MFSPV, "Received timestamp: $ts")
            Log.v(TAG_MFSPV, "System timestamp: $currentTs")

            if (ts == currentTs) {
                Log.v(TAG_MFSPV, "Timestamp validated!")
            } else {
                Log.v(TAG_MFSPV, "Timestamp not validated!")
                return null
            }

            /**
             * Verify signature
             *
             * First, it is necessary to extract the bytes from each field. The message was concatenated like this:
             * timestamp + mac + pseudoId + itsMessage
             * The timestamp is not necessary to extract because it was already previously extracted.
             */

            // -----------------
            // 1. Extract fields
            // -----------------

            val pseudoIdBytes = itsMessage.bytes.sliceArray(
                readPseudoIdStart until readPseudoIdEnd)  // read 20 bytes

            val macBytes = itsMessage.bytes.sliceArray(
                readMacStart until readMacEnd)  // read 20 bytes

            val itsMessageBytes = itsMessage.bytes.sliceArray(
                readItsMessageStart until itsMessage.bytes.size)

            // ---------------------------------
            // 2. Perform and compare signatures
            // ---------------------------------
            val contentToVerify: ByteArray = ByteUtils.concatenateByteArrays( // vararg
                pseudoIdBytes,     // PID
                regionalKey,
                itsMessageBytes,   // m
                tsBytes      // Ts
            )

            val md = MessageDigest.getInstance(hashAlgorithm)
            val calculatedMac : ByteArray = md.digest(contentToVerify)

            val calculatedMacLeastSignificant20bytes = ByteArray(20)

            // SHA 256 has 32 bytes, we only want the last 20 (from 12 to 32)
            System.arraycopy(
                calculatedMac, 12,
                calculatedMacLeastSignificant20bytes, 0, 20)

            Log.v(TAG_MFSPV, "pseudoIdBytes: ${pseudoIdBytes.contentToString()}")
            Log.v(TAG_MFSPV, "pseudoIdBytes size: ${pseudoIdBytes.size}")

            Log.v(TAG_MFSPV, "regionalKey: ${regionalKey.contentToString()}")
            Log.v(TAG_MFSPV, "regionalKey size: ${regionalKey.size}")

            Log.v(TAG_MFSPV, "itsMessage.bytes: ${itsMessageBytes.contentToString()}")
            Log.v(TAG_MFSPV, "itsMessage.bytes size: ${itsMessageBytes.size}")

            Log.v(TAG_MFSPV, "tsBytes: ${tsBytes.contentToString()}")
            Log.v(TAG_MFSPV, "tsBytes size: ${tsBytes.size}")

            return if (calculatedMacLeastSignificant20bytes.contentEquals(macBytes)) {
                Log.i(TAG_MFSPV, "Message successfully verified! The calculated signature is equal to the received one, mac verified with success.")
                Log.d(TAG_MFSPV, "MFSPV [isMessageValid] returning.")

                ITSMessage(itsMessageBytes)
            } else {
                Log.d(TAG_MFSPV, "The calculated signature is not equal to the received one, mac not verified.")
                Log.d(TAG_MFSPV, "MFSPV [isMessageValid] returning.")

                null
            }

        } catch (e: Exception) {
            Log.w(TAG_MFSPV, "Exception thrown in verifyMessage, exception message: ${e.message}.\nReturning null.")
            return null
        }
    }

}