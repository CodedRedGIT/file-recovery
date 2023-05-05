# file-recovery
## Description
File Recovery is a project aimed at recovering deleted files from a USB device. It utilizes block-level analysis to restore deleted files from a specific partition. The project is specifically configured to recover .pptx files from an EXT3 partition.

The process of recovering deleted files involves examining the blocks of data on the partition and identifying file signatures that match the specified file type (.pptx in this case). By analyzing the block data, the project attempts to reconstruct the deleted files and save them to a specified location.

## Features
- Block-level analysis: The project performs in-depth analysis of data blocks on the specified partition to identify and recover deleted files.
- Support for .pptx files: The project is optimized to recover deleted PowerPoint (.pptx) files. It focuses on identifying and reconstructing files of this specific format.
- EXT3 partition support: The project is designed to work with EXT3 partitions, which are commonly used in Linux systems. It can recover deleted files from this particular type of partition.
- File signature matching: By comparing the content of data blocks to known file signatures, the project can accurately identify files of the specified format.
- File reconstruction: Once a deleted file is identified, the project attempts to reconstruct it using the available block data. It then saves the recovered file to a specified location.

## Implementation
Block-level analysis is a technique used in the File Recovery project to recover deleted files from a USB device. This section provides an overview of the different types of blocks used in the analysis: direct blocks, indirect blocks, double indirect blocks, and triple indirect blocks.

### Direct Blocks
In a file system, data is stored in fixed-size blocks or sectors. Direct blocks refer to the data blocks that contain the actual content of a file. Each file has a limited number of direct blocks that store its data. When a file is deleted, the corresponding direct blocks are marked as free space, but the actual data remains intact until overwritten by new files.

During block-level analysis, the project scans the partition and identifies direct blocks that contain file signatures matching the specified file type (.pptx in this case). It then attempts to reconstruct the deleted files using the data from these blocks.

### Indirect Blocks
To store larger files, the file system uses indirect blocks. An indirect block contains a list of block addresses that point to additional data blocks. Instead of storing the file data directly, an indirect block acts as a pointer to the blocks holding the actual data.

During the analysis, if the project encounters an indirect block associated with a deleted file, it follows the block addresses within the indirect block to retrieve the actual data blocks. It then applies the same reconstruction process to recover the deleted file.

### Double Indirect Blocks
In situations where the file size exceeds the capacity of a single indirect block, the file system employs double indirect blocks. A double indirect block contains a list of block addresses that point to indirect blocks. Each indirect block, in turn, contains block addresses pointing to the data blocks.

During block-level analysis, if a deleted file is found to have a double indirect block, the project recursively follows the block addresses in the double indirect block to retrieve the indirect blocks and subsequently the data blocks. It continues the file reconstruction process using the data from these blocks.

### Triple Indirect Blocks
In cases where the file size surpasses the limit of a double indirect block, the file system employs triple indirect blocks. A triple indirect block contains a list of block addresses that point to double indirect blocks, which further point to indirect blocks, and finally to data blocks.

During the analysis, if a deleted file is discovered to have a triple indirect block, the project recursively follows the block addresses in the triple indirect block to retrieve the double indirect blocks, indirect blocks, and data blocks. The project continues the file reconstruction process using the data obtained from these blocks.

By employing block-level analysis and traversing through direct blocks, indirect blocks, double indirect blocks, and triple indirect blocks, the File Recovery project aims to reconstruct and recover deleted files from the specified partition.

## File Signatures and Identification
File signatures, also known as magic numbers or file headers, are unique patterns of bytes located at the beginning of a file. They serve as a distinctive identifier for specific file types. During the block-level analysis, the File Recovery project relies on these file signatures to identify and recover .pptx files.

### .pptx File Signature
A .pptx file is a Microsoft PowerPoint presentation file in the Office Open XML format. The file signature for .pptx files is a sequence of bytes that indicates the file's format and distinguishes it from other file types. The typical file signature for .pptx files is:
`50 4B 03 04`
This hexadecimal sequence corresponds to the ASCII values of "PK" (50 4B), followed by two additional bytes (03 04) that indicate the version of the Office Open XML format.

During the block-level analysis, the File Recovery project searches for this file signature in the data blocks of the specified partition. When a block contains this signature, it considers the block as a potential candidate for a deleted .pptx file and proceeds with the reconstruction process.

### File Reconstruction Process
Once the File Recovery project identifies a potential .pptx file based on the file signature, it initiates the file reconstruction process. The reconstruction process involves gathering the necessary data blocks containing the file's content and metadata and combining them to restore the file.

The project follows the file system's structure to retrieve the required blocks. It starts with the direct blocks containing the file's content. If the file is large and requires indirect, double indirect, or triple indirect blocks, the project recursively follows the block pointers to retrieve the necessary data.

The retrieved data blocks are then combined to form the reconstructed .pptx file. The project ensures the proper arrangement of blocks to maintain the file's integrity and correctness.

Once the file is successfully reconstructed, the File Recovery project saves the recovered .pptx file to the specified output directory, making it available for further use.

### Limitations and Considerations
It's important to note that the File Recovery project relies heavily on the accuracy of file signatures to identify and reconstruct .pptx files. If a file signature is missing or corrupted, it may result in unsuccessful file recovery. Additionally, if the deleted file has been partially overwritten or fragmented, the project's ability to recover the file may be compromised.

Furthermore, the File Recovery project is specifically configured to recover .pptx files from EXT3 partitions. It may not be suitable for other file types or file systems. For recovering different file types or from different partition types, modifications and adjustments to the project would be necessary.

It's recommended to create a backup of the partition or device before performing any file recovery operations to avoid potential data loss or further damage to the deleted files.

By leveraging file signatures and implementing the file reconstruction process, the File Recovery project aims to recover .pptx files from the specified EXT3 partition, facilitating the retrieval of deleted Microsoft PowerPoint presentations.

## Installation
To use the File Recovery project, follow these steps:

1. Clone the project repository from GitHub: `git clone https://github.com/CodedRedGIT/file-recovery.git`
2. Run makefile `make`
3. Make sure you have the necessary access permissions to read the USB device and the specified partition.

## Usage
1. Connect the USB device to your computer.
2. Make sure to add a .pptx file to this deviced then delete it.
3. Identify the device and partition you want to recover files from. You can use tools like fdisk to list the available devices and partitions.
4. Run the program with elevated permissions if necessary `sudo ./program`
5. You will be prompted for the device_path: The path to the USB device (e.g., /dev/sdb).
6. Then prompted for the output_directory: The directory where the recovered files will be saved.

- The project will start analyzing the specified partition and attempt to recover the deleted .pptx files. The recovered files will be saved in the specified output_directory.

## Contributing
Contributions to the File Recovery project are welcome. If you encounter any issues or have suggestions for improvements, please open an issue on the GitHub repository.

## Disclaimer
The File Recovery project is provided as-is without any warranty. The developers and contributors are not responsible for any data loss or damage that may occur during the use of this project. Use it at your own risk.

Please exercise caution when using the project and ensure that you have proper backups in place before attempting any file recovery operations.
