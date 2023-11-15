package pt.isel.meic.tfm.itsmobileapp.security_protocols.mfspv.models

/**
 * Class representing a PID (Pseudo-Identity)
 *
 * Pseudo-identities can hide the real identity of the ITS-S from other ITS stations and prevent tracking attacks
 */
class PseudoIdentityMFSPV(
    val bytes : ByteArray
) {
    override fun toString(): String {
        return "PseudoIdentity(bytes=${bytes.contentToString()})"
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is PseudoIdentityMFSPV) return false

        return bytes.contentEquals(other.bytes)
    }

    override fun hashCode(): Int {
        return bytes.contentHashCode()
    }

}
