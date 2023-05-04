#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/statvfs.h>
#include <sys/stat.h>

/**************************************************************************************
 * Function: finalizeFile
 * Description: Finalizes a file by determining its actual size and writing it to the output file.
 *              It also adds trailing zeros and double zeros as needed to align to 4-byte boundaries.
 * Parameters:
 *    - fileBlocks: A vector containing the block numbers of the file.
 *    - blockSize: The size of each block.
 *    - usb_fd: The file descriptor of the USB device.
 *    - out_fd: The file descriptor of the output file.
 * Returns:
 *    - The actual size of the file in bytes.
 **************************************************************************************/
off_t finalizeFile(const std::vector<int> &fileBlocks, int blockSize, int usb_fd, int out_fd)
{
    char buffer[4096];

    off_t fileSize = (fileBlocks.size() - 1) * blockSize;
    int lastBlock = fileBlocks.back();
    struct statvfs vfs;

    if (fstatvfs(usb_fd, &vfs) != 0)
    {
        std::cerr << "Failed to get file system information.\n";
        exit(1);
    }

    off_t lastBlockSize = vfs.f_frsize;
    fileSize += lastBlockSize;

    // Calculate the actual file size from the last block
    off_t lastBlockOffset = lseek(usb_fd, lastBlock * blockSize, SEEK_SET);
    if (lastBlockOffset == -1)
    {
        std::cerr << "Failed to seek to the last block.\n";
        exit(1);
    }

    ssize_t bytesRead = read(usb_fd, buffer, blockSize);
    if (bytesRead < 0)
    {
        std::cerr << "Failed to read the last block from the USB device.\n";
        exit(1);
    }

    // Find the index of the last non-zero byte
    int lastNonZeroIndex = -1;
    for (int i = bytesRead - 1; i >= 0; --i)
    {
        if (buffer[i] != 0)
        {
            lastNonZeroIndex = i;
            break;
        }
    }

    // Check if the last four bytes are all zeros
    bool endsWithZeros = false;
    if (lastNonZeroIndex >= 3 && buffer[lastNonZeroIndex] == 0 && buffer[lastNonZeroIndex - 1] == 0 &&
        buffer[lastNonZeroIndex - 2] == 0 && buffer[lastNonZeroIndex - 3] == 0)
    {
        endsWithZeros = true;
    }

    // Calculate the actual file size based on the last non-zero byte
    off_t actualFileSize = 0;
    if (lastNonZeroIndex != -1)
    {
        actualFileSize = lastNonZeroIndex + 1;
    }

    // Write the actual file data to the output file
    ssize_t bytesWritten = write(out_fd, buffer, actualFileSize);
    if (bytesWritten < 0)
    {
        std::cerr << "Failed to write the actual file data to the output file.\n";
        exit(1);
    }

    // Update the file size
    fileSize -= (lastBlockSize - actualFileSize);

    // Calculate the number of zeros needed to round up to a multiple of 4 bytes
    int zerosNeeded = (4 - (actualFileSize % 4)) % 4;

    // If the file does not end with "00 00 00 00", add the necessary zeros to the file
    if (!endsWithZeros)
    {
        char zeros[4] = {0, 0, 0, 0};

        bytesWritten = write(out_fd, zeros, zerosNeeded);
        if (bytesWritten < 0)
        {
            std::cerr << "Failed to write the trailing zeros to the output file.\n";
            exit(1);
        }

        fileSize += zerosNeeded;
    }

    // Check if the last two bytes are "00 00"
    bool endsWithDoubleZeros = false;
    if (actualFileSize >= 2 && buffer[actualFileSize - 2] == 0 && buffer[actualFileSize - 1] == 0)
    {
        endsWithDoubleZeros = true;
    }

    // If the file does not end with "00 00", add "00 00 00 00" to the end
    if (!endsWithDoubleZeros)
    {
        char doubleZeros[4] = {0, 0, 0, 0};

        bytesWritten = write(out_fd, doubleZeros, 4);
        if (bytesWritten < 0)
        {
            std::cerr << "Failed to write the double zeros to the output file.\n";
            exit(1);
        }

        fileSize += 4;
    }

    return fileSize;
}