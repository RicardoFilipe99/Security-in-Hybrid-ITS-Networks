package pt.isel.meic.tfm.itsmobileapp.security_protocols.dlapp.biometric_device

/**
 * Interface definition for the Biometric Device
 *
 * Each Biometric Device (BD) verifies the user identity by calculating the biometric login information
 */
interface IBiometricDevice {
    /**
     *  Each user logs in to the biometric device, using his/her biometric data
     */
    fun authenticateUser(): Boolean
}