#ifndef _SOS_EXT2_H
#define _SOS_EXT2_H

#include <vfs.h>
//Ref: http://www.nongnu.org/ext2-doc/ext2.html

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_DIRECT_BLOCKS 12

// sb.state
#define EXT2_VALID_FS	1 // unmounted cleanly
#define EXT2_ERROR_FS	2 // errors detected


// sb.errors
#define EXT2_ERRORS_CONTINUE	1 //	continue as if nothing happened
#define EXT2_ERRORS_RO			2 //	remount read-only
#define EXT2_ERRORS_PANIC		3 // 	cause a kernel panic

// sb.rev_level
#define EXT2_GOOD_OLD_REV	0	// Revision 0
#define EXT2_DYNAMIC_REV	1	// Revision 1 with variable inode sizes, extended attributes, etc.

#define EXT2_GOOD_OLD_FIRST_INO 11
#define EXT2_GOOD_OLD_INODE_SIZE 128

#define EXT2_FIRST_INO(sb) ((sb).rev_level == 0?EXT2_GOOD_OLD_FIRST_INO:(sb).first_ino)

#define EXT2_INODE_SIZE(sb) ((sb).rev_level == 0?EXT2_GOOD_OLD_INODE_SIZE:(sb).inode_size)

// creator os
#define EXT2_OS_LINUX	0	// Linux
#define EXT2_OS_HURD	1	// GNU HURD
#define EXT2_OS_MASIX	2	// MASIX
#define EXT2_OS_FREEBSD	3	// FreeBSD
#define EXT2_OS_LITES	4	// Lites

// features compat
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001	
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002	 
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008	
#define EXT2_FEATURE_COMPAT_RESIZE_INO		0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020

// features incompat
#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001	
#define EXT2_FEATURE_INCOMPAT_FILETYPE	  0x0002	 
#define EXT3_FEATURE_INCOMPAT_RECOVER	  0x0004	 
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008	 
#define EXT2_FEATURE_INCOMPAT_META_BG	  0x0010	 

//features ro-compat
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001	
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002	
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004	


/* Super block struct. */
typedef struct ext2_superblock_s {
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block; // actually is where sb resides
	uint32_t log_block_size;
	uint32_t log_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t wtime;

	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;

	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;

	uint16_t def_resuid;
	uint16_t def_resgid;

	// below are extended info (rev_level >= 1.0)
	/* EXT2_DYNAMIC_REV */
	uint32_t first_ino;
	uint16_t inode_size;
	uint16_t block_group_nr;
	uint32_t feature_compat;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;

	uint8_t uuid[16];
	uint8_t volume_name[16];

	uint8_t last_mounted[64];

	uint32_t algo_bitmap;

	/* Performance Hints */
	uint8_t prealloc_blocks;
	uint8_t prealloc_dir_blocks;
	uint16_t _padding;

	/* Journaling Support */
	uint8_t journal_uuid[16];
	uint32_t journal_inum;
	uint32_t jounral_dev;
	uint32_t last_orphan;

	/* Directory Indexing Support */
	uint32_t hash_seed[4];
	uint8_t def_hash_version;
	uint16_t _padding_a;
	uint8_t _padding_b;

	/* Other Options */
	uint32_t default_mount_options;
	uint32_t first_meta_bg;
	uint8_t _unused[760];

} __attribute__ ((packed)) ext2_superblock_t;

/* Block group descriptor. */
typedef struct ext2_group_desc_s {
	uint32_t block_bitmap;
	uint32_t inode_bitmap;		// block no. of inode bitmap
	uint32_t inode_table;
	uint16_t free_blocks_count;
	uint16_t free_inodes_count;
	uint16_t used_dirs_count;
	uint16_t pad;
	uint8_t reserved[12];
} __attribute__ ((packed)) ext2_group_desc_t;

/* File Types */
#define EXT2_S_IFSOCK	0xC000
#define EXT2_S_IFLNK	0xA000
#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFBLK	0x6000
#define EXT2_S_IFDIR	0x4000
#define EXT2_S_IFCHR	0x2000
#define EXT2_S_IFIFO	0x1000

/* setuid, etc. */
#define EXT2_S_ISUID	0x0800
#define EXT2_S_ISGID	0x0400
#define EXT2_S_ISVTX	0x0200

/* rights */
#define EXT2_S_IRUSR	0x0100
#define EXT2_S_IWUSR	0x0080
#define EXT2_S_IXUSR	0x0040
#define EXT2_S_IRGRP	0x0020
#define EXT2_S_IWGRP	0x0010
#define EXT2_S_IXGRP	0x0008
#define EXT2_S_IROTH	0x0004
#define EXT2_S_IWOTH	0x0002
#define EXT2_S_IXOTH	0x0001

/*
 *  * Constants relative to the data blocks
 *   */
#define EXT2_NDIR_BLOCKS        12
#define EXT2_IND_BLOCK          EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK         (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK         (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS           (EXT2_TIND_BLOCK + 1)

// reserved inodes
#define EXT2_BAD_INO			1	// bad blocks inode
#define EXT2_ROOT_INO			2	// root directory inode
#define EXT2_ACL_IDX_INO		3	// ACL index inode (deprecated?)
#define EXT2_ACL_DATA_INO		4	// ACL data inode (deprecated?)
#define EXT2_BOOT_LOADER_INO	5	// boot loader inode
#define EXT2_UNDEL_DIR_INO		6	// undelete directory inode

//flags
#define EXT2_SECRM_FL       0x00000001  // secure deletion
#define EXT2_UNRM_FL        0x00000002  // record for undelete
#define EXT2_COMPR_FL       0x00000004  // compressed file
#define EXT2_SYNC_FL        0x00000008  // synchronous updates
#define EXT2_IMMUTABLE_FL   0x00000010  // immutable file
#define EXT2_APPEND_FL      0x00000020  // append only
#define EXT2_NODUMP_FL      0x00000040  // do not dump/delete file
#define EXT2_NOATIME_FL     0x00000080  // do not update .i_atime
// -- Reserved for compression usage --
#define EXT2_DIRTY_FL       0x00000100  // Dirty (modified)
#define EXT2_COMPRBLK_FL    0x00000200  // compressed blocks
#define EXT2_NOCOMPR_FL     0x00000400  // access raw compressed data
#define EXT2_ECOMPR_FL      0x00000800  // compression error
// -- End of compression flags --
#define EXT2_BTREE_FL       0x00001000  // b-tree format directory
#define EXT2_INDEX_FL       0x00001000  // hash indexed directory
#define EXT2_IMAGIC_FL      0x00002000  // AFS directory
#define EXT3_JOURNAL_DATA_FL    0x00004000  // journal file data
#define EXT2_RESERVED_FL    0x80000000  // reserved for ext2 library


/* It represents an inode in an inode table on disk. */
typedef struct ext2_inode_s {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;			// file length in byte.
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t links_count;
	uint32_t sectors;
	uint32_t flags;
	uint32_t osd1;
	uint32_t block[EXT2_N_BLOCKS];
	uint32_t generation;
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t faddr;
	uint8_t osd2[12];
} __attribute__ ((packed)) ext2_inode_t;

// dirent.file_type
#define EXT2_FT_UNKNOWN		0	// Unknown File Type
#define EXT2_FT_REG_FILE	1	// Regular File
#define EXT2_FT_DIR			2	// Directory File
#define EXT2_FT_CHRDEV		3	// Character Device
#define EXT2_FT_BLKDEV		4	// Block Device
#define EXT2_FT_FIFO		5	// Buffer File
#define EXT2_FT_SOCK		6	// Socket File
#define EXT2_FT_SYMLINK		7	// Symbolic Link

/* Represents directory entry on disk. */
typedef struct ext2_dirent_s {
	uint32_t inode;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	char name[];		/* Actually a set of characters, at most 255 bytes */
} __attribute__ ((packed)) ext2_dirent_t;


typedef struct {
	uint32_t block_no;
	uint32_t last_use;
	uint8_t  dirty;
	uint8_t *block;
} ext2_disk_cache_entry_t;

typedef int (*ext2_block_io_t) (void *, uint32_t, uint8_t *);

typedef struct ext2_fs_info_s {
	int groups_count;
	uint32_t start_sector;
	uint32_t block_size; // in bytes
    uint32_t sectors_per_block;
    uint32_t inodes_per_sector;
	uint32_t frag_size; 
} ext2_fs_info_t;

typedef struct ext2_group_info_s {
	ext2_group_desc_t desc;
	// useful info
	int has_backup_sb;

	int group_no;
	uint32_t start_block;
	uint32_t end_block;
	uint32_t free_start_block; // first available data block id
	uint32_t free_start_ino; // first available ino
	uint32_t free_end_ino; // last available ino
} ext2_group_info_t;

class Ext2Fs: public FileSystem {
    public:
        void init(dev_t dev);
        dentry_t * lookup(inode_t * dir, dentry_t *) override;

        ssize_t read(File *, char * buf, size_t count, off_t * offset) override;
        ssize_t write(File *, const char * buf, size_t, off_t *offset) override;
        int readdir(File *, dentry_t *, filldir_t) override;

    private:
        dev_t _dev;  // disk dev
        dev_t _pdev; 
		ext2_superblock_t _sb;
		ext2_group_info_t* _gdt;
		ext2_fs_info_t _fs;

		void read_inode(inode_t* ip);
		ext2_inode_t* iget(uint32_t ino);
		int iread(ext2_inode_t*, off_t off, char* buf, size_t count);
        int iread_block(ext2_inode_t*, uint32_t bid, off_t off, char*, size_t);
        uint32_t iget_indirect_block_no(ext2_inode_t* eip, uint32_t bid);
        int read_whole_block(uint32_t bid, char*);

        uint32_t inode_occupied_blocks(ext2_inode_t* eip);
};

FileSystem* create_ext2fs(const void*);

#endif

