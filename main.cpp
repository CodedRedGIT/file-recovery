#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sys/statvfs.h>
#include <algorithm>
#include <sstream>
#include <array>
#include <memory>
#include <iomanip>
#include <sys/stat.h>
#include "BlockIO.h"
#include "BlockRecovery.h"

// Function to open the USB device and return the file descriptor
int openUSBDevice(const std::string &devicePath)
{
    int usb_fd = open(devicePath.c_str(), O_RDONLY);
    if (usb_fd == -1)
    {
        std::cerr << "Failed to open USB device.\n";
        exit(1);
    }
    return usb_fd;
}

// Function to open the output file and return the file descriptor
int openOutputFile(const std::string &outputPath)
{
    int out_fd = open(outputPath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (out_fd == -1)
    {
        std::cerr << "Failed to open output file.\n";
        exit(1);
    }
    return out_fd;
}

off_t determineFileSize(const std::vector<int> &fileBlocks, int blockSize, int usb_fd, int out_fd)
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

// Function to recover the file from the USB device
void recoverFile(const std::string &usbDevicePath, const std::string &outputPath, const std::string &fileTypeSignature)
{
    int blockSize = 4096;
    char buffer[4096];
    int usb_fd = openUSBDevice(usbDevicePath);
    int out_fd = openOutputFile(outputPath);

    const unsigned char *signature = reinterpret_cast<const unsigned char *>(fileTypeSignature.c_str());
    int startBlock = BlockRecovery::findFirstBlockOfType(usb_fd, fileTypeSignature);

    std::vector<int> directBlocks = BlockRecovery::findDirectBlocks(usb_fd, startBlock, 4096, 12);
    std::vector<int> totalBlocks = directBlocks;

    // Write the direct blocks to the output file
    for (int i = 0; i < directBlocks.size(); ++i)
    {
        int block = directBlocks[i];
        std::cout << "\ti_block[" << i << "] = " << block << "\n";
        std::cout.flush();

        ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
        if (bytesRead < 0)
        {
            std::cerr << "Failed to read direct block " << block << " from USB device.\n";
            exit(1);
        }

        writeBlock(out_fd, buffer, 4096);
    }

    // Get indirect block if exists
    int indirectBlock;
    if (directBlocks.size() == 12 && directBlocks.back() != '0')
    {
        indirectBlock = BlockRecovery::findIndirectBlock(usb_fd, startBlock, blockSize, directBlocks.back() + 1);
        std::cout << "\ti_block[12] = " << indirectBlock << "\n";

        std::vector<int> directBlockNumbers = BlockRecovery::getBlockNumbersFromIndirect(usb_fd, indirectBlock, blockSize);

        // Write the indirect blocks to the output file
        for (int i = 0; i < directBlockNumbers.size(); ++i)
        {
            int block = directBlockNumbers[i];

            if (block == 0)
                continue;

            ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
            if (bytesRead < 0)
            {
                std::cerr << "Failed to read direct block " << block << " from USB device.\n";
                exit(1);
            }
            writeBlock(out_fd, buffer, 4096);
            totalBlocks.push_back(block);
        }

        // Get and write double indirect blocks
        if (directBlockNumbers[directBlockNumbers.size() - 1] != 0)
        {
            std::cout << "\ti_block[13] = " << (indirectBlock + 1) << "\n";
            std::vector<int> blocksFromDoubleIndirect = BlockRecovery::findDoubleIndirectBlocks(usb_fd, indirectBlock + 1, 4096);
            for (int i = 0; i < blocksFromDoubleIndirect.size(); ++i)
            {
                int block = blocksFromDoubleIndirect[i];

                if (block == 0)
                    continue;

                ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
                if (bytesRead < 0)
                {
                    std::cerr << "Failed to read direct block " << block << " from USB device.\n";
                    exit(1);
                }
                writeBlock(out_fd, buffer, 4096);
                totalBlocks.push_back(block);
            }

            // Write the triple indirect blocks
            if (blocksFromDoubleIndirect.back() != 0)
            {
                std::cout << "\ti_block[14] = " << (indirectBlock + 2) << "\n";
                std::vector<int> tripleIndirectBlocks = BlockRecovery::findTripleIndirectBlocks(usb_fd, blocksFromDoubleIndirect.back() + 1, blockSize);

                for (int i = 0; i < tripleIndirectBlocks.size(); ++i)
                {
                    int block = tripleIndirectBlocks[i];

                    if (block == 0)
                        continue;

                    ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
                    if (bytesRead < 0)
                    {
                        std::cerr << "Failed to read direct block " << block << " from USB device.\n";
                        exit(1);
                    }
                    writeBlock(out_fd, buffer, 4096);
                    totalBlocks.push_back(block);
                }
            }
            else
                std::cout << "\ti_block[14] = 0\n";
        }
        else
        {
            std::cout << "\ti_block[13] = 0\n";
            std::cout << "\ti_block[14] = 0\n";
        }
    }
    else
    {
        std::cout << "\ti_block[12] = 0\n";
        std::cout << "\ti_block[13] = 0\n";
        std::cout << "\ti_block[14] = 0\n";
    }

    off_t fileSize = determineFileSize(totalBlocks, 4096, usb_fd, out_fd);

    // Set the file size on the output file
    if (ftruncate(out_fd, fileSize) != 0)
    {
        std::cerr << "Failed to set file size on output file.\n";
        exit(1);
    }

    close(usb_fd);
    close(out_fd);
}

bool setFilePermissions(const std::string &filePath)
{
    int result = chmod(filePath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (result != 0)
    {
        std::cerr << "Failed to set file permissions for: " << filePath << std::endl;
        return false;
    }
    return true;
}

int main()
{
    bool inProduction = false;

    std::string usbDevicePath;
    std::string outputPath;

    if (inProduction)
    {
        std::cout << "Enter the USB device path: ";
        std::getline(std::cin, usbDevicePath);

        std::cout << "Enter the output file path: ";
        std::getline(std::cin, outputPath);
    }
    else
    {
        usbDevicePath = "/dev/sdd";
        outputPath = "/home/codedred/Desktop/FinalProject/outfile.pptx";
    }

    std::string fileTypeSignature = "\x50\x4B\x03\x04";

    std::cout << "File recovery started.\n\n";
    recoverFile(usbDevicePath, outputPath, fileTypeSignature);

    if (setFilePermissions(outputPath))
    {
        std::cout << "\n\nFile permissions set successfully." << std::endl;
    }

    std::cout << "File copying completed.\n";
    return 0;
}
