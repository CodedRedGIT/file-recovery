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

// Function to recover the file from the USB device
void recoverFile(const std::string &usbDevicePath, const std::string &outputPath, const std::string &fileTypeSignature)
{
    int blockSize = 4096;
    char buffer[4096];
    int usb_fd = openUSBDevice(usbDevicePath);
    int out_fd = openOutputFile(outputPath);

    const unsigned char *signature = reinterpret_cast<const unsigned char *>(fileTypeSignature.c_str());
    std::cout << "Looking for first block...\n";
    int startBlock = BlockRecovery::findFirstBlockOfType(usb_fd, fileTypeSignature);
    std::cout << "Found!\n";

    std::cout << "Looking for direct blocks...\n";
    std::vector<int> directBlocks = BlockRecovery::findDirectBlocks(usb_fd, startBlock, 4096, 12);
    std::cout << "Found!\n";

    // Write the direct blocks to the output file
    for (int i = 0; i < directBlocks.size(); ++i)
    {
        int block = directBlocks[i];
        std::cout << "Processing direct block " << block << " (" << i + 1 << "/" << directBlocks.size() << ")\n";
        std::cout.flush();

        ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096); //
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
        std::cout << "Looking for indirect block...\n";
        indirectBlock = BlockRecovery::findIndirectBlock(usb_fd, startBlock, blockSize, directBlocks.back() + 1);
        std::cout << "Found!\n";

        std::vector<int> directBlockNumbers = BlockRecovery::getBlockNumbersFromIndirect(usb_fd, indirectBlock, blockSize);

        // Write the indirect blocks to the output file
        for (int i = 0; i < directBlockNumbers.size(); ++i)
        {
            int block = directBlockNumbers[i];

            if (block == 0)
                continue; // Skip processing if blockNumber is 0

            // std::cout << "Processing indirect block " << block << " (" << i + 1 << "/" << directBlockNumbers.size() << ")\n";
            std::cout.flush();

            ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
            if (bytesRead < 0)
            {
                std::cerr << "Failed to read direct block " << block << " from USB device.\n";
                exit(1);
            }
            writeBlock(out_fd, buffer, 4096);
        }

        // Get and write double indirect blocks
        if (directBlockNumbers[directBlockNumbers.size() - 1] != 0)
        {
            std::vector<int> indirectBlocks = BlockRecovery::findDoubleIndirectBlocks(usb_fd, indirectBlock + 1, 4096);
            for (int i = 0; i < indirectBlocks.size(); ++i)
            {
                int block = indirectBlocks[i];

                if (block == 0)
                    continue; // Skip processing if blockNumber is 0

                // std::cout << "Processing indirect block " << block << " (" << i + 1 << "/" << directBlockNumbers.size() << ")\n";
                std::cout.flush();

                ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096);
                if (bytesRead < 0)
                {
                    std::cerr << "Failed to read direct block " << block << " from USB device.\n";
                    exit(1);
                }
                writeBlock(out_fd, buffer, 4096);
            }

            // Write the triple indirect blocks
            if (indirectBlocks.back() != 0)
            {
                std::vector<int> tripleIndirectBlocks = BlockRecovery::findTripleIndirectBlocks(usb_fd, indirectBlocks.back() + 1, blockSize);
                std::cout << "Triple Indirect Blocks:" << std::endl;
                for (int block : tripleIndirectBlocks)
                {
                    std::cout << block << std::endl;
                }
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
                }
            }
        }
    }

    std::cout << "File recovery completed.\n";
    std::cout.flush();

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
        usbDevicePath = "/dev/sdc";
        outputPath = "/home/codedred/Desktop/FinalProject/outfile.pptx";
    }

    std::string fileTypeSignature = "\x50\x4B\x03\x04";

    std::cout << "File recovery started.\n";
    recoverFile(usbDevicePath, outputPath, fileTypeSignature);

    if (setFilePermissions(outputPath))
    {
        std::cout << "File permissions set successfully." << std::endl;
    }

    std::cout << "File copying completed.\n";
    return 0;
}
