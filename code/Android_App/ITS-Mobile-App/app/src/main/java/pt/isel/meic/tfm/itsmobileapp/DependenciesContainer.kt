package pt.isel.meic.tfm.itsmobileapp

import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.IReceiveMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.ISendMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher.IMessageDispatcher
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol

/**
 * Container for all high-level dependencies (e.g., services)
 */
interface DependenciesContainer {
    val messageDispatcher: IMessageDispatcher
    val sendMessageService : ISendMessageService
    val receiveMessageService : IReceiveMessageService
    val securityProt : ISecurityProtocol
}