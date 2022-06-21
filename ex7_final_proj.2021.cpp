#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256

// ============================================================================

class FsFile
{
public:
	int file_size;

	int block_in_use;

	int index_block;

	int block_size;

	// offset in current block
	int block_offset;

	FsFile(int _block_size)
	{

		file_size = 0;

		block_in_use = -1;

		block_size = _block_size;

		index_block = -1;

		block_offset = 0;
	}

	int getfile_size()
	{

		return file_size;
	}
};

// ============================================================================

class FileDescriptor
{

	string file_name;

	FsFile *fs_file;

	bool inUse;

public:
	FileDescriptor(string FileName, FsFile *fsi)
	{

		file_name = FileName;

		fs_file = fsi;

		inUse = true;
	}

	~FileDescriptor()
	{
		delete fs_file;
	}

	string getFileName()
	{

		return file_name;
	}

	bool isInUse()
	{
		return inUse;
	}

	void setInUse(bool _inUse)
	{
		inUse = _inUse;
	}

	FsFile *getFsFile()
	{
		return fs_file;
	}
};

// ============================================================================

class MainFile
{
public:
	int fd;
	FileDescriptor *fileDescriptor;

	MainFile(int _fd, FileDescriptor *_fileDescriptor)
	{
		fd = _fd;
		fileDescriptor = _fileDescriptor;
	}

	~MainFile()
	{
		delete fileDescriptor;
	}
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk
{

	FILE *sim_disk_fd;

	bool is_formated;

	// BitVector - "bit" (int) vector, indicate which block in the disk is free

	//              or not.  (i.e. if BitVector[0] == 1 , means that the

	//             first block is occupied.

	int BitVectorSize;

	int *BitVector;

	// filename and one fsFile.

	vector<MainFile *> MainDir;
	vector<FileDescriptor *> OpenFileDescriptors;

	int direct_enteris; // unused

	int block_size;

	int maxSize;

	int freeBlocks;

	// ------------------------------------------------------------------------
public:
	fsDisk()
	{

		sim_disk_fd = fopen(DISK_SIM_FILE, "r+");

		assert(sim_disk_fd);

		for (int i = 0; i < DISK_SIZE; i++)
		{

			int ret_val = fseek(sim_disk_fd, i, SEEK_SET);

			ret_val = fwrite("\0", 1, 1, sim_disk_fd);

			assert(ret_val == 1);
		}

		fflush(sim_disk_fd);

		direct_enteris = 0;

		block_size = 0;

		freeBlocks = 0;
		is_formated = false;
	}

	~fsDisk()
	{
		delete[] BitVector;
		for (auto file : MainDir)
		{
			delete file;
		}
	}

	// ------------------------------------------------------------------------
	void listAll()
	{

		int i = 0;

		for (auto fd : MainDir)
		{

			cout << "index: " << i << ": FileName: " << fd->fileDescriptor->getFileName() << " , isInUse: "
				 << fd->fileDescriptor->isInUse() << endl;
			i++;
		}

		char bufy;

		cout << "Disk content: '";

		for (i = 0; i < DISK_SIZE; i++)
		{

			int ret_val = fseek(sim_disk_fd, i, SEEK_SET);

			ret_val = fread(&bufy, 1, 1, sim_disk_fd);

			cout << bufy;
		}

		cout << "'" << endl;
	}

	// ------------------------------------------------------------------------
	void fsFormat(int blockSize = 4)
	{
		block_size = blockSize;
		freeBlocks = DISK_SIZE / block_size;
		BitVector = new int[freeBlocks];
		for (int i = 0; i < freeBlocks; ++i)
		{
			BitVector[i] = 0;
		}
		BitVectorSize = freeBlocks;

		for (int i = 0; i < DISK_SIZE; i++)
		{

			int ret_val = fseek(sim_disk_fd, i, SEEK_SET);

			ret_val = fwrite("\0", 1, 1, sim_disk_fd);

			assert(ret_val == 1);
		}

		fflush(sim_disk_fd);
		is_formated = true;
	}

	// ------------------------------------------------------------------------
	int CreateFile(string fileName)
	{
		if (!is_formated)
		{
			return -1; // error: disk must be formatted
		}
		int fd = getNewFd();
		if (fd == -1)
		{
			return -1;
		}
		FsFile *fsFile = new FsFile(block_size);
		int idx_block = getNewBlock();
		if (idx_block == -1)
			return -1;
		fsFile->index_block = idx_block;

		FileDescriptor *fileDescriptor = new FileDescriptor(fileName, fsFile);
		MainFile *file = new MainFile(fd, fileDescriptor);

		fileDescriptor->setInUse(true);
		OpenFileDescriptors.push_back(fileDescriptor);
		MainDir.push_back(file);
		return fd;
	}

	int getNewFd()
	{
		if (freeBlocks <= 1)
		{
			return -1; // disk is full
		}
		int fd = -1;
		for (MainFile *file : MainDir)
		{
			if (file->fd > fd)
			{
				fd = file->fd;
			}
		}
		return fd + 1;
	}

	int getNewBlock()
	{
		if (freeBlocks == 0)
			return -1;
		for (int i = 0; i < BitVectorSize; ++i)
		{
			if (!BitVector[i])
			{
				BitVector[i] = 1;
				freeBlocks--;
				return i;
			}
		}
		return -1;
	}

	FileDescriptor *getFileDesc(int fd)
	{
		for (MainFile *file : MainDir)
		{
			if (file->fd == fd)
			{
				return file->fileDescriptor;
			}
		}
		return nullptr;
	}

	// ------------------------------------------------------------------------
	int OpenFile(string fileName)
	{
		for (MainFile *file : MainDir)
		{
			FileDescriptor *fd = file->fileDescriptor;
			if (fd->getFileName() == fileName)
			{
				if (fd->isInUse())
				{
					return -1; // error: already in use
				}
				fd->setInUse(true);
				return file->fd;
			}
		}
		return -1; // error: file does not exist
	}

	// ------------------------------------------------------------------------
	string CloseFile(int fd)
	{
		if (fd < 0 || fd >= MainDir.size())
			return "-1"; // error: invalid fd

		MainFile *_file = nullptr;
		for (MainFile *file : MainDir)
		{
			if (file->fd == fd)
			{
				_file = file;
				break;
			}
		}
		if (_file == nullptr)
		{
			return "-1"; // error: file does not exist
		}

		FileDescriptor *fileDesc = _file->fileDescriptor;

		if (!fileDesc->isInUse())
			return "-1"; // error: file already closed

		// remove file from open fds
		for (auto c = OpenFileDescriptors.begin(); c != OpenFileDescriptors.end(); ++c)
		{
			if (*c == fileDesc)
			{
				OpenFileDescriptors.erase(c);
				break;
			}
		}
		fileDesc->setInUse(false);
		return fileDesc->getFileName();
	}

	// ------------------------------------------------------------------------
	int WriteToFile(int fd, char *buf, int len)
	{
		if (!is_formated)
		{
			return -1; //error: disk not initialized
		}
		if (fd < 0)
		{
			return -1; // error: invalid fd
		}
		FileDescriptor *fileDesc = getFileDesc(fd);
		if (fileDesc == nullptr)
			return -1; // error: file does not exist
		if (!fileDesc->isInUse())
			return -1; // error: file not opened

		FsFile *file = fileDesc->getFsFile();

		int bytesWrote = 0;

		for (int i = 0; i < len; i++)
		{
			if (file->file_size == block_size * block_size) {
				break; // file is full
			}
			if (file->block_in_use == -1 || file->block_offset >= block_size)
			{
				// open a new block and keep writing
				int new_block = getNewBlock();
				if (new_block == -1)
				{
					break; // disk is full
				}
				file->block_offset = 0;
				file->block_in_use = new_block;
				int blocks = file->file_size / block_size;
				int idx_offset = block_size * file->index_block + blocks;
				fseek(sim_disk_fd, idx_offset, SEEK_SET);
				char b = new_block;
				fwrite(&b, 1, 1, sim_disk_fd);
				fflush(sim_disk_fd);
			}

			int offset = block_size * file->block_in_use + file->block_offset++;
			fseek(sim_disk_fd, offset, SEEK_SET);
			fwrite(buf + i, 1, 1, sim_disk_fd);
			file->file_size++;
			bytesWrote++;
		}
		fflush(sim_disk_fd);
		return bytesWrote;
	}
	// ------------------------------------------------------------------------
	/**
	 * Return 1 if del is success else 0
	 */
	int DelFile(string FileName)
	{
		// FileDescriptor *fileDesc = getFileDesc(fd);
		FileDescriptor *fileDesc = nullptr;
		int fd = -1;
		for (MainFile *_f : MainDir)
		{
			if (_f->fileDescriptor->getFileName() == FileName)
			{
				fileDesc = _f->fileDescriptor;
				fd = _f->fd;
			}
		}
		if (fileDesc == nullptr)
			return -1; // error: file does not exist
		if (!fileDesc->isInUse())
			return -1; // error: file not opened

		FsFile *file = fileDesc->getFsFile();
		int blocks_amount = file->file_size / block_size;
		if (file->file_size % block_size != 0)
		{
			blocks_amount++;
		}
		int idx_block_offset = file->index_block * block_size;
		BitVector[file->index_block] = 0;
		// Clean the sim_disk_file from the file's data
		for (int i = 0; i < blocks_amount; ++i)
		{
			char currBlock;
			fseek(sim_disk_fd, idx_block_offset + i, SEEK_SET);
			fread(&currBlock, 1, 1, sim_disk_fd);

			int block_idx = currBlock * block_size;

			for (int j = 0; j < block_size; ++j)
			{
				fseek(sim_disk_fd, block_idx + j, SEEK_SET);
				fwrite("\0", 1, 1, sim_disk_fd);
			}
			BitVector[currBlock] = 0;
			freeBlocks++;
		}
		for (int i = 0; i < blocks_amount; ++i)
		{
			fseek(sim_disk_fd, idx_block_offset + i, SEEK_SET);
			fwrite("\0", 1, 1, sim_disk_fd);

		}
		fflush(sim_disk_fd);
		for (auto c = OpenFileDescriptors.begin(); c != OpenFileDescriptors.end(); c++) // C++
		{
			if (*c == fileDesc)
			{
				OpenFileDescriptors.erase(c);
				break;
			}
		}
		for (auto c = MainDir.begin(); c != MainDir.end(); ++c)
		{
			if ((*c)->fd == fd)
			{
				delete *c;
				MainDir.erase(c);
				break;
			}
		}
		return fd;
	}

	// ------------------------------------------------------------------------
	int ReadFromFile(int fd, char *buf, int len)
	{
		FileDescriptor *fileDesc = getFileDesc(fd);
		if (fileDesc == nullptr)
			return -1; // error: file does not exist
		if (!fileDesc->isInUse())
			return -1; // error: file not opened

		FsFile *file = fileDesc->getFsFile();

		int blocks_amount = file->file_size / block_size;
		if (file->file_size % block_size != 0)
		{
			blocks_amount++;
		}

		int bytesRead = 0;
		if (len > file->file_size)
		{
			len = file->file_size;
		}

		int idx_block = block_size * file->index_block;
		int idx_block_offset = 0;

		char currBlock;
		int curr_block_offset = 0;
		while (bytesRead < len)
		{
			if (bytesRead % block_size == 0)
			{
				curr_block_offset = 0;
				fseek(sim_disk_fd, idx_block + idx_block_offset++, SEEK_SET);
				fread(&currBlock, 1, 1, sim_disk_fd);
			}
			fseek(sim_disk_fd, (block_size * currBlock) + curr_block_offset++, SEEK_SET);
			fread(buf + bytesRead++, 1, 1, sim_disk_fd);
		}
		buf[len] = 0;
		return len;
	}
};

int main()
{
	int blockSize;
	int direct_entries;
	string fileName;
	char str_to_write[DISK_SIZE];
	char str_to_read[DISK_SIZE];
	int size_to_read;
	int _fd;
	int l;

	fsDisk *fs = new fsDisk();
	int cmd_;
	while (1)
	{
		cin >> cmd_;
		switch (cmd_)
		{
			case 0: // exit
				delete fs;
				exit(0);
				break;

			case 1: // list-file
				fs->listAll();
				break;

			case 2: // format
				cin >> blockSize;
				fs->fsFormat(blockSize);
				break;

			case 3: // creat-file
				cin >> fileName;
				_fd = fs->CreateFile(fileName);
				cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
				break;

			case 4: // open-file
				cin >> fileName;
				_fd = fs->OpenFile(fileName);
				cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
				break;

			case 5: // close-file
				cin >> _fd;
				fileName = fs->CloseFile(_fd);
				cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
				break;

			case 6: // write-file
				cin >> _fd;
				cin >> str_to_write;
				l = fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
				cout << "Wrote: " << l << endl;
				break;

			case 7: // read-file
				cin >> _fd;
				cin >> size_to_read;
				fs->ReadFromFile(_fd, str_to_read, size_to_read);
				cout << "ReadFromFile: " << str_to_read << endl;
				break;

			case 8: // delete file
				cin >> fileName;
				_fd = fs->DelFile(fileName);
				cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
				break;
			default:
				break;
		}
	}
}