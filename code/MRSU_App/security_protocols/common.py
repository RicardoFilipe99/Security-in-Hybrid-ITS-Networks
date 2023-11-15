from enum import Enum


class SecurityProtocolEnum(Enum):
    NONE = 0
    DLAPP = 1
    MFSPV = 2

    def __str__(self):
        return self.name
