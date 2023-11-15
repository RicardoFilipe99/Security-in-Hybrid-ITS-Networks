package pt.isel.meic.tfm.itsmobileapp.security_protocols.no_security

import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import pt.isel.meic.tfm.itsmobileapp.security_protocols.Common

const val TAG_NOSECURITY = "ITSMobApp-NoSecurity"

/**
 * Only returns what it receives (no security)
 */
class NoSecurity : ISecurityProtocol {

    override val protocolDesignation  : Common.ProtocolEnum = Common.ProtocolEnum.NONE

    override fun applySecurityProtocolEncapsulation(itsMessage : ITSMessage): ITSMessage {
        Log.i(TAG_NOSECURITY, "Security not being used, so encapsulation is not needed.")
        return itsMessage
    }

    override fun applySecurityProtocolDecapsulation(itsMessage : ITSMessage): ITSMessage {
        Log.i(TAG_NOSECURITY, "Security not being used, so decapsulation is not needed.")
        return itsMessage
    }

}