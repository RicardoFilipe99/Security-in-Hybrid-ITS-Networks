package pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.message_generator;

import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage

/**
 * Interface definition for an ITS Message Generator
 */
interface IMessageGenerator {

    fun generateCAM(): ITSMessage

    fun generateDENM(): ITSMessage

}
