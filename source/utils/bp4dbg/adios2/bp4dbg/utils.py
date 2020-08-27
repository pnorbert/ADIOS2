LocalValueDim = 18446744073709551613

dataTypes = {
    -1: 'unknown',
    0: 'byte',
    1: 'short',
    2: 'integer',
    4: 'long',

    50: 'unsigned_byte',
    51: 'unsigned_short',
    52: 'unsigned_integer',
    54: 'unsigned_long',

    5: 'real',
    6: 'double',
    7: 'long_double',

    9: 'string',
    10: 'complex',
    11: 'double_complex',
    12: 'string_array'
}

dataTypeSize = {
    -1: 0,
    0: 1,
    1: 2,
    2: 4,
    4: 8,

    50: 1,
    51: 2,
    52: 4,
    54: 8,

    5: 4,
    6: 8,
    7: 16,

    9: 0,
    10: 8,
    11: 16,
    12: 0
}


def GetTypeName(typeID):
    name = dataTypes.get(typeID)
    if name is None:
        name = "unknown type"
    return name


def GetTypeSize(typeID):
    size = dataTypeSize.get(typeID)
    if size is None:
        size = 0
    return size


CharacteristicNames = {
    0: 'value',
    1: 'min',
    2: 'max',
    3: 'offset',
    4: 'dimensions',
    5: 'var_id',
    6: 'payload_offset',
    7: 'file_index',
    8: 'time_index',
    9: 'bitmap',
    10: 'stat',
    11: 'transform_type',
    12: 'minmax'
}


def GetCharacteristicName(cID):
    name = CharacteristicNames.get(cID)
    if name is None:
        name = "unknown characteristic"
    return name


def GetCharacteristicDataLength(cID, typeID):
    name = CharacteristicNames.get(cID)
    if (name == 'value' or name == 'min' or
            name == 'max' or name == 'minmax'):
        return dataTypeSize[typeID]
    elif (name == 'offset' or name == 'payload_offset'):
        return 8
    elif (name == 'file_index' or name == 'time_index'):
        return 4
    else:
        return 0


# Read Header info 64 bytes
# fileType: Data, Metadata, Index Table
# return: status, sortedMetadata  (bool, bool)
def ReadHeader(f, fileSize, fileType):
    status = True
    if fileSize < 64:
        print("ERROR: Invalid " + fileType + ". File is smaller "
              "than the header (64 bytes)")
        return False
    header = f.read(64)
    hStr = header.decode('ascii')

    versionStr = hStr[0:32].replace('\0', ' ')
    major = hStr[32]
    minor = hStr[33]
    micro = hStr[34]
#    unused = hStr[35]

    endianValue = header[36]
    if endianValue == 0:
        endian = 'yes'
    elif endianValue == 1:
        endian = ' no'
    else:
        print("ERROR: byte 28 must be 0 or 1 to indicate endianness of "
              "the data. It is however {0} in this file".format(
                  endianValue))
        status = False

    bpversion = int(header[37])
    active = int(header[38])
    if active == 0:
        activeStr = ' no'
    else:
        activeStr = 'yes'

    unsortedMetadata = int(header[39])
    if unsortedMetadata == 0:
        sortedMetadataStr = 'yes'
        sortedMetadataFlag = True
    elif unsortedMetadata == 1:
        sortedMetadataStr = ' no'
        sortedMetadataFlag = False
    else:
        print("ERROR: byte 39 must be 0 or 1 to indicate if the"
              "meatadata is sorted. It is however {0} in this file".format(
                  unsortedMetadata))
        sortedMetadataStr = ' ? '
        sortedMetadataFlag = False
        status = False
    # 40..63 unused

    print("-----------------------------------------------------------"
          "-----------------------------------------------------------"
          "--------")
    print("|        Version string            | Major | Minor | Patch "
          "| unused | Endian | BP version | Active | Sorted "
          "|    unused      |")
    print("|          32 bytes                |   1B  |   1B  |   1B  "
          "|   1B   |   1B   |     1B     |   1B   |   1B   "
          "|      24B       |")
    print("+----------------------------------------------------------"
          "-----------------------------------------------------------"
          "-------+")
    print("| {0} |   {1}   |   {2}   |   {3}   |        |  {4}   "
          "|      {5}     |  {6}   |  {7}   |                |".format(
              versionStr, major, minor, micro, endian, bpversion, 
              activeStr, sortedMetadataStr))
    print("-----------------------------------------------------------"
          "-----------------------------------------------------------"
          "--------")
    return status, sortedMetadataFlag


if __name__ == "__main__":
    print("ERROR: Utility main program is bp4dbg.py")
