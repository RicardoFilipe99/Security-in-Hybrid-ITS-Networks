package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.models

/**
 * Class representing an HashChain
 *
 * A hash chain consists in a set of secret keys to perform a Message Authentication Code (MAC)
 */
class HashChain(
    val hashChain: Array<ByteArray>
) {
    override fun toString(): String {
        val sb = StringBuilder()

        sb.append("HashChain(hashChain=").append("\n")
        for (bytes in hashChain)
            sb.append(bytes.contentToString()).append("\n")
        sb.append(")")

        return sb.toString()
    }
}

