#ifndef BLOCKIO_H
#define BLOCKIO_H

#include <iostream>

ssize_t readBlock(int usb_fd, int blockNumber, char *buffer, int blockSize);
void writeBlock(int out_fd, const char *buffer, int blockSize);

#endif // BLOCKIO_H
