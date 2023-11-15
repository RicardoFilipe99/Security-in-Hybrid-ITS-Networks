package pt.isel.meic.tfm.itsmobileapp.security_protocols

import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage


/**
 * Interface definition to use ITS Security Protocols
 */
interface ISecurityProtocol {

    val protocolDesignation : Common.ProtocolEnum

    /**
     * Method to perform security protocol protection (add protocol's specific fields)
     */
    fun applySecurityProtocolEncapsulation(itsMessage: ITSMessage) : ITSMessage

    /**
     * Method to perform security protocol deprotection (i.e., verifying message and remove protocol's specific fields)
     */
    fun applySecurityProtocolDecapsulation(itsMessage: ITSMessage) : ITSMessage?

}