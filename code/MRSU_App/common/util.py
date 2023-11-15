import array
import os


def generate_random_byte_array(length):
    """
    Generates random a byte arrays with the specified length.

    @param length: byte array's length
    @return: byte array
    """
    return os.urandom(length)


def create_signed_byte_array_from(byte_array):
    """
    Convert to signed byte array (as we do in kotlin).

    @param byte_array: unsigned byte array
    @return: signed byte array
    """
    return array.array('b', byte_array)


def xor_byte_arrays(array1, array2):
    """
    Performs XOR between two byte arrays

    @param array1: byte array 1
    @param array2: byte array 2
    @return: byte array 1 XOR byte array 2
    """
    result = bytearray()
    for b1, b2 in zip(array1, array2):
        result.append(b1 ^ b2)
    return result
