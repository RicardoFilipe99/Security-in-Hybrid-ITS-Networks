package pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.message_generator

import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.common.convertHexStringToByteArray

/**
 * Implementation Message Generator
 */
class MessageGeneratorImpl : IMessageGenerator {

    override fun generateCAM(): ITSMessage {
        TODO("Not yet implemented, ASN1 library tool necessary")
    }

    override fun generateDENM(): ITSMessage {
        TODO("Not yet implemented, ASN1 library tool necessary")
    }

}

/**
 * Implementation Fake Message Generator (it returns hardcoded (previously generated) messages)
 */
class MessageGeneratorFakeImpl : IMessageGenerator {

    // "0202000000C803E8005997D7ED8CBB585E401401401430D5407F00000000003E713A800BFFE5FFF80013FE01400540042D693A401AD2748000000000",

    private val camsHexList = listOf<String>(
        "0202000000C803E8005997D7ED8CBB585E401401401430D5400000000000003E713A800BFFE5FFF800"
    )

    override fun generateCAM(): ITSMessage {
        return ITSMessage(convertHexStringToByteArray(camsHexList.random()))
    }

    override fun generateDENM(): ITSMessage {
        TODO("")
    }

}
