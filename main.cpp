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

// Determine the file size from the file blocks
off_t determineFileSize(const std::vector<int> &fileBlocks, int blockSize, int usb_fd)
{
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

    return fileSize;
}

// Function to recover the file from the USB device
void recoverFile(const std::string &usbDevicePath, const std::string &outputPath, const std::string &fileTypeSignature)
{
    int blockSize = 4096;
    int usb_fd = openUSBDevice(usbDevicePath);
    int out_fd = openOutputFile(outputPath);

    const unsigned char *signature = reinterpret_cast<const unsigned char *>(fileTypeSignature.c_str());
    std::cout << "Looking for first block...\n";
    int startBlock = BlockRecovery::findFirstBlockOfType(usb_fd, fileTypeSignature);
    std::cout << "Found!\n";

    std::cout << "Looking for direct blocks...\n";
    std::vector<int> directBlocks = BlockRecovery::findDirectBlocks(usb_fd, startBlock, 4096, 12); // Block size is set to 4096
    std::cout << "Found!\n";

    std::cout << "Looking for indirect block...\n";
    int indirectBlock;
    if (directBlocks.size() == 12 && directBlocks.back() != '0')
    {
        indirectBlock = BlockRecovery::findIndirectBlock(usb_fd, startBlock, blockSize, directBlocks.back() + 1);
        std::cout << "Found!\n";
    }
    else
        std::cout << "Not Found!\n";

    char buffer[4096]; // Block size is set to 4096

    // Write the direct blocks to the output file
    for (int i = 0; i < directBlocks.size(); ++i)
    {
        int block = directBlocks[i];
        std::cout << "Processing direct block " << block << " (" << i + 1 << "/" << directBlocks.size() << ")\n";
        std::cout.flush(); // Flush standard output

        // Read the direct block
        ssize_t bytesRead = readBlock(usb_fd, block, buffer, 4096); // Block size is set to 4096
        if (bytesRead < 0)
        {
            std::cerr << "Failed to read direct block " << block << " from USB device.\n";
            exit(1);
        }

        // Write the direct block to the output file
        writeBlock(out_fd, buffer, 4096);
    }

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
    directBlocks.insert(directBlocks.end(), directBlockNumbers.begin(), directBlockNumbers.end());

    if (directBlockNumbers[directBlockNumbers.size() - 1] != 0)
    {
        std::vector<int> indirectBlocks = BlockRecovery::findDoubleIndirectBlocks(usb_fd, indirectBlock + 1, 4096);
        std::cout << "Indirect Blocks:" << std::endl;
        for (int block : indirectBlocks)
        {
            std::cout << block << std::endl;
        }
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
        directBlocks.insert(directBlocks.end(), indirectBlocks.begin(), indirectBlocks.end());
    }

    off_t fileSize = determineFileSize(directBlocks, 4096, usb_fd);

    // Set the file size on the output file
    if (ftruncate(out_fd, fileSize) != 0)
    {
        std::cerr << "Failed to set file size on output file.\n";
        exit(1);
    }

    std::cout << "File recovery completed.\n";
    std::cout.flush();

    if (chmod(outputPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) != 0)
    {
        std::cerr << "Failed to change file permissions.\n";
        exit(1);
    }

    close(usb_fd);
    close(out_fd);
}

int main()
{
    std::string usbDevicePath = "/dev/sdc";
    std::string outputPath = "/home/codedred/Desktop/FinalProject/outfile.pptx";
    std::string fileTypeSignature = "\x50\x4B\x03\x04";

    std::cout << "File recovery started.\n";
    recoverFile(usbDevicePath, outputPath, fileTypeSignature);

    std::string zipPath = "/home/codedred/Desktop/FinalProject/text.zip";

    // Open the input (outputPath) and output (zipPath) files
    std::ifstream inputFile(outputPath, std::ios::binary);
    std::ofstream outputFile(zipPath, std::ios::app); // Open in append mode

    if (!inputFile)
    {
        std::cerr << "Failed to open the input file.\n";
        return 1;
    }

    if (!outputFile)
    {
        std::cerr << "Failed to open the output file.\n";
        return 1;
    }

    // Copy the contents from the input file to the output file
    outputFile << inputFile.rdbuf();

    // Close the input and output files
    inputFile.close();
    outputFile.close();

    std::cout << "File copying completed.\n";
    return 0;
}