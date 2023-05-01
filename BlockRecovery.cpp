#include "BlockRecovery.h"
#include "BlockIO.h"

/**************************************************************************************
 * Function: findDirectBlocks
 * Description: Finds the direct blocks of a file on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - startBlock: The starting block number.
 *    - blockSize: The size of each block.
 *    - numDirectBlocks: The number of direct blocks to find.
 * Returns:
 *    - A vector containing the direct block numbers.
 **************************************************************************************/
int BlockRecovery::findFirstBlockOfType(int usb_fd, const std::string &fileTypeSignature)
{
    const int bufferSize = 4096;
    char buffer[bufferSize];

    int blockNumber = 0;
    int bytesRead = 0;

    while ((bytesRead = read(usb_fd, buffer, bufferSize)) == bufferSize)
    {
        if (memcmp(buffer, fileTypeSignature.c_str(), fileTypeSignature.size()) == 0)
        {
            std::cerr << "Block = " << blockNumber << ".\n";
            return blockNumber;
        }

        blockNumber++;
    }

    // Check the last block
    if (bytesRead > 0)
    {
        const char *match = std::search(buffer, buffer + bytesRead, fileTypeSignature.begin(), fileTypeSignature.end());
        if (match != buffer + bytesRead)
        {
            int position = match - buffer;
            int foundBlock = blockNumber + (position / bufferSize);
            std::cerr << "Block = " << foundBlock << ".\n";
            return foundBlock;
        }
    }

    std::cerr << "Could not find the specified file type on the USB device.\n";
    exit(1);
}

/**************************************************************************************
 * Function: findDirectBlocks
 * Description: Finds the direct blocks of a file on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - startBlock: The starting block number.
 *    - blockSize: The size of each block.
 *    - numDirectBlocks: The number of direct blocks to find.
 * Returns:
 *    - A vector containing the direct block numbers.
 **************************************************************************************/
std::vector<int> BlockRecovery::findDirectBlocks(int usb_fd, int startBlock, int blockSize, int numDirectBlocks)
{
    std::vector<int> directBlocks;
    char buffer[blockSize];
    ssize_t bytesRead;

    // Read and write the data for the next numDirectBlocks blocks (direct blocks)
    for (int i = 0; i < numDirectBlocks; ++i)
    {
        bytesRead = readBlock(usb_fd, startBlock + i, buffer, blockSize);

        // Check if any non-zero byte is present in the block
        bool isEmpty = true;
        for (int j = 0; j < bytesRead; ++j)
        {
            if (buffer[j] != '\0')
            {
                isEmpty = false;
                break;
            }
        }

        if (isEmpty)
        {
            break; // Stop reading further blocks if the block is empty
        }

        directBlocks.push_back(startBlock + i);

        std::cout << "Block data read and written for block number " << startBlock + i << "\n";
        std::cout.flush(); // Flush standard output
    }

    std::cout << "Direct blocks identified: " << directBlocks.size() << "\n";
    std::cout.flush(); // Flush standard output

    return directBlocks;
}

/**************************************************************************************
 * Function: findIndirectBlock
 * Description: Finds the indirect block with the specified value on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - startBlock: The starting block number.
 *    - blockSize: The size of each block.
 *    - targetValue: The value to search for in the indirect blocks.
 * Returns:
 *    - The block number of the found indirect block.
 **************************************************************************************/

int BlockRecovery::findIndirectBlock(int usb_fd, int startBlock, int blockSize, int targetValue)
{
    const int bufferSize = 4096;
    char buffer[bufferSize];
    int blockNumber = startBlock - 10000; // Initialize blockNumber as 0
    int bytesRead = 0;

    unsigned char targetBytesValue[4];
    targetBytesValue[0] = (targetValue >> 0) & 0xFF;
    targetBytesValue[1] = (targetValue >> 8) & 0xFF;
    targetBytesValue[2] = (targetValue >> 16) & 0xFF;
    targetBytesValue[3] = (targetValue >> 24) & 0xFF;

    while ((bytesRead = readBlock(usb_fd, blockNumber, buffer, blockSize)) == blockSize)
    {
        if (memcmp(buffer, targetBytesValue, sizeof(targetBytesValue)) == 0)
        {
            std::cout << "Block found at block number " << blockNumber << std::endl;
            return blockNumber;
        }

        ++blockNumber;
    }

    std::cerr << "Could not find the indirect block with the specified value.\n";
    exit(1);
}

/**************************************************************************************
 * Function: findDoubleIndirectBlocks
 * Description: Finds the direct blocks of double indirect block a file on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - doubleIndirectBlockNumber: The block number of the double indirect block.
 *    - blockSize: The size of each block.
 * Returns:
 *    - A vector containing the direct block numbers.
 **************************************************************************************/
std::vector<int> BlockRecovery::findDoubleIndirectBlocks(int usb_fd, int doubleIndirectBlockNumber, int blockSize)
{
    const int bufferSize = 4096;
    char buffer[bufferSize];
    int bytesRead = 0;
    int blockCount = blockSize / sizeof(int);

    std::cout << "DOUBLE INDIRECT: " << doubleIndirectBlockNumber << std::endl;

    std::vector<int> directBlockNumbers;
    if (lseek(usb_fd, doubleIndirectBlockNumber * blockSize, SEEK_SET) == -1)
    {
        std::cerr << "Error seeking to double indirect block number " << doubleIndirectBlockNumber << std::endl;
        exit(1);
    }

    bytesRead = read(usb_fd, buffer, bufferSize);
    if (bytesRead == -1)
    {
        std::cerr << "123---Error reading double indirect block number " << doubleIndirectBlockNumber << std::endl;
        exit(1);
    }

    int doubleIndirectBlockNumbers[blockCount];
    memcpy(doubleIndirectBlockNumbers, buffer, bytesRead);

    for (int i = 0; i < blockCount; ++i)
    {
        int indirectBlockNumber = doubleIndirectBlockNumbers[i];
        if (indirectBlockNumber == 0)
        {
            continue; // Skip if the indirect block number is 0
        }

        if (lseek(usb_fd, indirectBlockNumber * blockSize, SEEK_SET) == -1)
        {
            std::cerr << "Error seeking to indirect block number " << indirectBlockNumber << std::endl;
            exit(1);
        }

        bytesRead = read(usb_fd, buffer, bufferSize);
        if (bytesRead == -1)
        {
            std::cerr << "Error reading indirect block number " << indirectBlockNumber << std::endl;
            exit(1);
        }

        int indirectBlockNumbers[blockCount];
        memcpy(indirectBlockNumbers, buffer, bytesRead);

        for (int j = 0; j < blockCount; ++j)
        {
            int directBlockNumber = indirectBlockNumbers[j];
            if (directBlockNumber == 0)
            {
                continue; // Skip if the direct block number is 0
            }

            directBlockNumbers.push_back(directBlockNumber);
        }
    }

    return directBlockNumbers;
}

/**************************************************************************************
 * Function: findTripleIndirectBlocks
 * Description: Finds the triple indirect blocks of a file on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - tripleIndirectBlock: The block number of the triple indirect block.
 *    - blockSize: The size of each block.
 * Returns:
 *    - A vector containing the triple indirect block numbers.
 **************************************************************************************/
std::vector<int> BlockRecovery::findTripleIndirectBlocks(int usb_fd, int tripleIndirectBlockNumber, int blockSize)
{
    const int bufferSize = 4096;
    char buffer[bufferSize];
    int bytesRead = 0;
    int blockCount = blockSize / sizeof(int);

    std::vector<int> directBlockNumbers;

    if (lseek(usb_fd, tripleIndirectBlockNumber * blockSize, SEEK_SET) == -1)
    {
        std::cerr << "Error seeking to triple indirect block number " << tripleIndirectBlockNumber << std::endl;
        exit(1);
    }

    bytesRead = read(usb_fd, buffer, bufferSize);
    if (bytesRead == -1)
    {
        std::cerr << "Error reading triple indirect block number " << tripleIndirectBlockNumber << std::endl;
        exit(1);
    }

    int tripleIndirectBlockNumbers[blockCount];
    memcpy(tripleIndirectBlockNumbers, buffer, bytesRead);

    for (int i = 0; i < blockCount; ++i)
    {
        int doubleIndirectBlockNumber = tripleIndirectBlockNumbers[i];
        if (doubleIndirectBlockNumber == 0)
        {
            continue; // Skip if the double indirect block number is 0
        }

        std::vector<int> doubleIndirectBlocks = findDoubleIndirectBlocks(usb_fd, doubleIndirectBlockNumber, blockSize);

        // Append the direct block numbers from the double indirect blocks
        directBlockNumbers.insert(directBlockNumbers.end(), doubleIndirectBlocks.begin(), doubleIndirectBlocks.end());
    }

    return directBlockNumbers;
}

/**************************************************************************************
 * Function: getBlockNumbersFromIndirect
 * Description: Retrieves the block numbers from the given indirect block on the USB device.
 * Parameters:
 *    - usb_fd: The file descriptor of the USB device.
 *    - indirectBlockNumber: The block number of the indirect block.
 *    - blockSize: The size of each block.
 * Returns:
 *    - A vector containing the direct block numbers.
 **************************************************************************************/
std::vector<int> BlockRecovery::getBlockNumbersFromIndirect(int usb_fd, int indirectBlockNumber, int blockSize)
{

    const int bufferSize = 4096;
    char buffer[bufferSize];
    int bytesRead = 0;
    int directBlockCount = blockSize / sizeof(int);

    if (lseek(usb_fd, indirectBlockNumber * blockSize, SEEK_SET) == -1)
    {
        std::cerr << "Error seeking to indirect block number " << indirectBlockNumber << std::endl;
        exit(1);
    }

    bytesRead = read(usb_fd, buffer, bufferSize);
    if (bytesRead == -1)
    {
        std::cerr << "Error reading indirect block number " << indirectBlockNumber << std::endl;
        exit(1);
    }

    int *blockNumbers = reinterpret_cast<int *>(buffer);
    std::vector<int> directBlockNumbers;

    for (int i = 0; i < directBlockCount; ++i)
    {
        directBlockNumbers.push_back(blockNumbers[i]);
    }

    return directBlockNumbers;
}