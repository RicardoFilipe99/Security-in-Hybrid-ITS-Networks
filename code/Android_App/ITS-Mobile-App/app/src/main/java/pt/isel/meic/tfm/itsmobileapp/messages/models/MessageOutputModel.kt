package pt.isel.meic.tfm.itsmobileapp.messages.models

import pt.isel.meic.tfm.itsmobileapp.security_protocols.Common
import java.time.Instant

enum class MessageType {
    CAM, DENM
}

class ScreenMessageOutput(

    private val itsMessageContent: ByteArray,
    private val timestamp: Instant,
    private val beingSent: Boolean,
    private val messageType: MessageType,
    private val securityProtocol: Common.ProtocolEnum

) : IMessageOutputModel {

    override val content: String
        get() = this.toString()

    override fun toString(): String {

        return if (beingSent)
            "A message was sent:\n" +
                    "Security=$securityProtocol,\n" +
                    "Message type=$messageType,\n" +
                    "Timestamp=$timestamp,\n" +
                    "Message content size=${itsMessageContent.size},\n" +
                    "Message content bytes='${itsMessageContent.contentToString()}')"

        else
            "A message was received:\n" +
                    "Security=$securityProtocol,\n" +
                    "Message type=$messageType,\n" +
                    "Timestamp=$timestamp,\n" +
                    "Message content size=${itsMessageContent.size},\n" +
                    "Message content bytes='${itsMessageContent.contentToString()}')"

    }

}
