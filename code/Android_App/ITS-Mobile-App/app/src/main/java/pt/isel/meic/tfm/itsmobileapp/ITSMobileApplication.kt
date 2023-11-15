package pt.isel.meic.tfm.itsmobileapp

import android.app.Application
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.IReceiveMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.ReceiveMessageServiceImpl
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.ISendMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.message_generator.MessageGeneratorFakeImpl
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.SendMessageServiceImpl
import pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher.IMessageDispatcher
import pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher.MessageDispatcherMQTTImpl
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ProtocolSetup
import pt.isel.meic.tfm.itsmobileapp.security_protocols.Common

/**
 * We define which Application to use in the AndroidManifest file,
 * currently 'ITSMobileApplication' is being used (Use ctrl+shift+f to find it).
 */

/**
 * ITSMobileApplicationPrototype - To be used when some components are still 'fake', e.g., message generator
 */
class ITSMobileApplication : DependenciesContainer, Application()  {

    /**
     * Specify the security protocol to be used
     */
    private val securityProtocolEnum : Common.ProtocolEnum = Common.ProtocolEnum.NONE // NONE, DLAPP or MFSPV

    private var securityProtocolObj : ISecurityProtocol = ProtocolSetup.getSecurityProtocol(securityProtocolEnum)

    /**
     * Specify if performance is being assessed
     */
    private val evaluationEnum : Evaluator.EvaluationEnum = Evaluator.EvaluationEnum.NONE

    /**
     * Define necessary components to the ViewModel
     */

    // Create an instance of type IReceiveMessageService
    override val receiveMessageService: IReceiveMessageService
        get() = ReceiveMessageServiceImpl(
            securityProtocol = securityProtocolObj,
            evaluation = evaluationEnum
        )

    // Create an instance of type IMessageDispatcher
    private val myMessageDispatcher : IMessageDispatcher = MessageDispatcherMQTTImpl(
        receiveService = receiveMessageService,
        evaluation = evaluationEnum
    )

    override val messageDispatcher: IMessageDispatcher
        get() = myMessageDispatcher

    // Create an instance of type ISendMessageService
    override val sendMessageService: ISendMessageService
        get() = SendMessageServiceImpl(
            securityProtocol = securityProtocolObj,
            messageGenerator =  MessageGeneratorFakeImpl(),
            dispatcher = messageDispatcher,
            camFrequency = 2.0,
            evaluation = evaluationEnum
        )

    // Create an instance of type ISendMessageService
    override val securityProt: ISecurityProtocol = securityProtocolObj
}