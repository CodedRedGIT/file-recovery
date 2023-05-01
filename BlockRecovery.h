#ifndef BLOCKRECOVERY_H
#define BLOCKRECOVERY_H
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

#include <vector>

class BlockRecovery
{
public:
    static int findFirstBlockOfType(int usb_fd, const std::string &fileTypeSignature);
    static std::vector<int> findDirectBlocks(int usb_fd, int startBlock, int blockSize, int numDirectBlocks);
    static int findIndirectBlock(int usb_fd, int startBlock, int blockSize, int targetValue);
    static std::vector<int> findDoubleIndirectBlocks(int usb_fd, int doubleIndirectBlock, int blockSize);
    static std::vector<int> findTripleIndirectBlocks(int usb_fd, int tripleIndirectBlock, int blockSize);
    static std::vector<int> getBlockNumbersFromIndirect(int usb_fd, int indirectBlockNumber, int blockSize);
};

#endif // BLOCKRECOVERY_H
