# File-System-Simulation

Authored by Mahmoud Zatari

==Description==

The program is a simulation of a disk file system, using indexed allocation
method, to simulate the actions performed when creating files, writing to them
or other related processes are used.

Data Structures:
1. Main Directory - vector of file pointers.
2. Open File Descriptor - vector of file descriptor pointers to each open file.

Classes:
1. FsFile Class - holds the information of files, including:
                file size, number of blocks in use, block size,
                index block and block offset.
2. File Descriptor Class - holds the file name and a pointer to 
                it's FsFile, with a boolean argument that indicates
                if the file is opened or not.
3. Main File Class - holds the file and it's descriptor.
4. fsDisk Class - simulating the disk and the operations of disk management.

Functions:

void listAll():
    Arguments: None.
    A function to print a list of the files on the disk, and the contents of
    the disk.
void fsFormat(int):
    Arguments: Block size.
    A function to format the disk.
int CreateFile(string):
    Arguments: File name.
    A function to create a new file on the system.
int getNewFd():
    Arguments: None.
    A function to give fd numbers to created files.
int getNewBlock():
    Arguments: None.
    A function to open a new block.
int OpenFile(string):
    Arguments: File name.
    A function to open files.
string CloseFile(int):
    Arguments: File descriptor.
    A function to close files.
int WriteToFile(int, char*, int):
    Arguments: File descriptor, a buffer holding the string to write to file,
    the string's length.
    A function to write strings to files.
int DelfFile(string):
    Arguments: File name.
    A function to delete existing files.
int ReadFromFile(int, char*, int):
    Arguments: File descriptor, a buffer and the amount of characters to read.
    A function to reads specific amount of characters from files.


==Program Files==

ex7_final_proj.2021.cpp - the file contains functions, classes and the
implementation of the file system simulator.


==How to compile?==

Create a text file in the same directory with the name DISK_SIM_FILE.txt
Compile: g++ ex7_final_proj.2021.cpp -o ex7_final_proj.2021
Run: ./ex7_final_proj.2021


==Input==

Via cmd, as specified in the excersise. every input consists of a number
that indicates the action to perform, followed by various types of parameters
depending on the action. To end the program, an output of '0' must be entered.


==Output==

Depending on the actions performed, the output would vary between creating files,
writing to them, reading from them, deleting the file system and multiple other
things, ending the program for example with the input of '0'.
