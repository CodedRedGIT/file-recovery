#include "BlockIO.h"
#include <unistd.h>
#include <fcntl.h>

ssize_t readBlock(int usb_fd, int blockNumber, char *buffer, int blockSize)
{
    off_t offset = blockNumber * blockSize;
    lseek(usb_fd, offset, SEEK_SET);
    ssize_t bytesRead = read(usb_fd, buffer, blockSize);
    if (bytesRead == -1)
    {
        std::cerr << "Failed to read block from USB device.\n";
        exit(1);
    }
    return bytesRead;
}

void writeBlock(int out_fd, const char *buffer, int blockSize)
{
    ssize_t bytesWritten = 0;
    ssize_t totalBytesWritten = 0;
    while (totalBytesWritten < blockSize)
    {
        bytesWritten = write(out_fd, buffer + totalBytesWritten, blockSize - totalBytesWritten);
        if (bytesWritten == -1)
        {
            std::cerr << "Failed to write block to output file.\n";
            exit(1);
        }
        totalBytesWritten += bytesWritten;

        // Flush data to disk
        if (fsync(out_fd) == -1)
        {
            std::cerr << "Failed to flush data to disk.\n";
            exit(1);
        }
    }
}
