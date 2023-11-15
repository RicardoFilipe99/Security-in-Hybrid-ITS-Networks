package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.biometric_device

/**
 * Prototype implementation of Biometric Device Interface, it always gives successful authentication
 */
class BiometricDeviceImpl : IBiometricDevice {
    override fun authenticateUser(): Boolean {
        return true
    }
}