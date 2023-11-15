package pt.isel.meic.tfm.itsmobileapp.messages.models

/**
 * Class that represents an ITS message (specifically it's bytes), either the original message
 * (e.g., CAM or DENM) or the message protected with a security protocol
 */
data class ITSMessage(val bytes : ByteArray) {

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as ITSMessage

        if (!bytes.contentEquals(other.bytes)) return false

        return true
    }

    override fun hashCode(): Int {
        return bytes.contentHashCode()
    }

}