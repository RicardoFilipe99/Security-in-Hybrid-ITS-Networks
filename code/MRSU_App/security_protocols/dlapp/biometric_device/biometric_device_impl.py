class BiometricDeviceImpl:
    """
    Prototype implementation of Biometric Device Interface, it always gives successful authentication
    """
    def __init__(self, return_value):
        self.return_value = return_value

    def authenticate_user(self,):
        return self.return_value
