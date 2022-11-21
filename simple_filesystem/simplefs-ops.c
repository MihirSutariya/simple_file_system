#include "simplefs-disk.c"

#include "simplefs-ops.h"
#include <stdio.h>
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	struct inode_t *ptr = (struct inode_t*) malloc(sizeof(struct inode_t));

	int inodenum =simplefs_allocInode();

	if(inodenum==-1){
		return -1;
	}
	for(int a1=0; a1<MAX_FILE_SIZE; a1++){
		ptr->direct_blocks[a1] = -1;
	}
	ptr->status =INODE_IN_USE;
	ptr->file_size = 0;
	int counter =0;
	while(counter<7 && filename[counter]!='\0'){
		ptr->name[counter] = filename[counter];
		counter+=1;
	}
	ptr->name[counter] = '\0';

	simplefs_writeInode(inodenum,ptr);

    return 0;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
	struct inode_t *ptr = (struct inode_t*) malloc(sizeof(struct inode_t));
	int innum = -1;
	for(int a1=0; a1<NUM_INODES; a1++){
		simplefs_readInode(a1,ptr);
		int check =1;
		for(int a2=0; a2<8; a2++){
			if(filename[a2]!=ptr->name[a2]){
				check = 0;
				break;
			}
		}
		if(check==1){
			innum = a1;
		}
	}
	if(innum==-1){
		return;
	}
	for(int a1 = 0; a1< MAX_FILE_SIZE; a1++){
		if(ptr->direct_blocks[a1]!=-1){
			simplefs_freeDataBlock(ptr->direct_blocks[a1]);
		}
	}
	simplefs_freeInode(innum);
}

int simplefs_open(char *filename){
	struct inode_t *ptr = (struct inode_t*) malloc(sizeof(struct inode_t));
	int innum = -1;
	for(int a1=0; a1<NUM_INODES; a1++){
		simplefs_readInode(a1,ptr);
		if(ptr->name == filename){
			innum = a1;
			break;
		}
	}
	if(innum==-1){
		return -1;
	}
	for(int a1=0; a1<MAX_OPEN_FILES; a1++){
		if(file_handle_array[a1].offset==-1){
			file_handle_array[a1].offset=0;
			file_handle_array[a1].inode_number=innum;
			return a1;
		}
	}
    return -1;
}

void simplefs_close(int file_handle){

	file_handle_array[file_handle].offset = -1;


}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	int total = nbytes;
	int off = file_handle_array[file_handle].offset+1;
	struct inode_t *ptr = (struct inode_t*) malloc(sizeof(struct inode_t));
	simplefs_readInode(file_handle_array[file_handle].inode_number,ptr);
	char* data = (char *)malloc((BLOCKSIZE)*sizeof(char));
	int index = off-1 - 64*(off/BLOCKSIZE);
	int counter =0;

	while((off+index)/BLOCKSIZE < MAX_FILE_SIZE && nbytes!=0){
		index = off-1 - 64*(off/BLOCKSIZE);
		int blocknum = ptr->direct_blocks[(off+index)/BLOCKSIZE];

		if(blocknum==-1){
			return total-nbytes;
		}
		simplefs_readDataBlock(blocknum,data);

		while(nbytes!=0 && index!=BLOCKSIZE){
			buf[counter] = *(data+index);	
			counter+=1;
			index+=1;		
			nbytes -=1;	
		}	
	}

    return total-nbytes;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
	int total = nbytes; 
	int off = file_handle_array[file_handle].offset+1;
	struct inode_t *ptr = (struct inode_t*) malloc(sizeof(struct inode_t));
	simplefs_readInode(file_handle_array[file_handle].inode_number,ptr);
	char* data = (char *)malloc((BLOCKSIZE)*sizeof(char));
	int index = off-1 - 64*(off/BLOCKSIZE);
	int counter =0;

	while((off+index)/BLOCKSIZE < MAX_FILE_SIZE && nbytes!=0){
		index = off-1 - 64*(off/BLOCKSIZE);
		int blocknum = ptr->direct_blocks[(off+index)/BLOCKSIZE];
		if(blocknum==-1){
			blocknum = simplefs_allocDataBlock();
			ptr->direct_blocks[(off+index)/BLOCKSIZE] = blocknum;
		}
		simplefs_readDataBlock(blocknum,data);
		while(nbytes!=0 && index!=BLOCKSIZE){
			*(data+index) = buf[counter];	
			counter+=1;
			index+=1;	
			nbytes -=1;		
		}	
		simplefs_writeDataBlock(blocknum,data);
	}
	ptr->file_size +=(total - nbytes);
	simplefs_writeInode(file_handle_array[file_handle].inode_number,ptr);
    return total-nbytes;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/

	file_handle_array[file_handle].offset+=nseek;
	if(file_handle_array[file_handle].offset>MAX_FILE_SIZE*BLOCKSIZE || file_handle_array[file_handle].offset<0){
		return -1;
	}
    return 0;
}