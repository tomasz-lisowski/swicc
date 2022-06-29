#pragma once
/**
 * @todo Handle the index rules for different file types somewhere.
 * Linear-Fixed: indices begin at the first record in the array.
 * Cyclic: indices begin at the last/selected record in the array.
 */

#include "swicc/common.h"
#include "swicc/fs/common.h"
#include <assert.h>
#include <stdint.h>

#define SWICC_DISK_MAGIC_LEN 16U
/**
 * Different file signatures to differentiate the endianness of the swICC FS
 * file.
 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SWICC_DISK_MAGIC                                                       \
    {                                                                          \
        0x00, 's', 'w', 'I', 'C', 'C', 0x91, 0xCC, '.', '.', '.', '.', 'F',    \
            'S', 0xF0, 0x0F                                                    \
    }
#elif __BYTE_ORDER == __BIG_ENDIAN
#define SWICC_DISK_MAGIC                                                       \
    {                                                                          \
        0x00, 's', 'w', 'I', 'C', 'C', 0x91, 0xCC, '.', '.', '.', '.', 'F',    \
            'S', 0x0F, 0xF0                                                    \
    }
#else
#error "Invalid endianness."
#endif
static_assert(sizeof((uint8_t[])SWICC_DISK_MAGIC) == SWICC_DISK_MAGIC_LEN,
              "Magic length macro not equal to the magic array length");

/* Representation of a LUT. */
typedef struct swicc_disk_lut_s
{
    uint8_t *buf1;
    uint8_t *buf2;

    uint32_t size_item1; /* Size of item in buffer 1. */
    uint32_t size_item2; /* Size of item in buffer 2. */

    /* Both buffers will be resized to contain the same amount of elements. */
    uint32_t count_max; /* Allocated size can hold this many items. */
    uint32_t count;     /* Number of items in the buffer. */
} swicc_disk_lut_st;

/* Representation of a tree in the root (forest). */
typedef struct swicc_disk_tree_s swicc_disk_tree_st;
struct swicc_disk_tree_s
{
    /**
     * The root-level trees like the MF and ADFs are attached to a single
     * linked-list which forms the forest.
     */
    swicc_disk_tree_st *next;
    uint8_t *buf;  /* This buffer holds the whole disk (including LUTs). */
    uint32_t size; /* Allocated size. */
    uint32_t len;  /* Occupied size. */
    swicc_disk_lut_st lutsid;
};

/* The in-memory struct storing a swICC FS disk. */
typedef struct swicc_disk_s
{
    swicc_disk_tree_st *root;
    swicc_disk_lut_st lutid; /* There is exactly one LUT for all IDs. */
} swicc_disk_st;

/**
 * Looking up trees by index is inefficient when trying to perform some action
 * for some range of consecutive trees so better to provide an iterator that can
 * be used to iterate through all trees or to some specific index in the most
 * efficient way.
 */
typedef struct swicc_disk_tree_iter_s
{
    uint8_t tree_idx; /* Index of the tree at the head of the iterator. */
    swicc_disk_tree_st *tree;
} swicc_disk_tree_iter_st;

/**
 * @brief Load a disk file (into memory).
 * @param disk
 * @param disk_path Path to the disk file.
 * @return Return code.
 */
swicc_ret_et swicc_disk_load(swicc_disk_st *const disk,
                             char const *const disk_path);

/**
 * @brief Unload the in-memory disk and frees any memory used for storing the
 * FS.
 * @param swicc_state
 */
void swicc_disk_unload(swicc_disk_st *const disk);

/**
 * @brief Save the disk as a swICC FS file to a specified file.
 * @param disk
 * @param disk_path Path where to save the disk file.
 * @return Return code.
 */
swicc_ret_et swicc_disk_save(swicc_disk_st const *const disk,
                             char const *const disk_path);

/**
 * @brief A callback for the 'foreach' iterator.
 * @param tree The tree inside which is the file.
 * @param file A file in the tree.
 * @param userdata Anything the user needs to access in their callback which is
 * not already provided.
 * @return Return code.
 */
typedef swicc_ret_et fs_file_foreach_cb(swicc_disk_tree_st *const tree,
                                        swicc_fs_file_st *const file,
                                        void *const userdata);
/**
 * @brief For every file in a given file, perform some operation.
 * @param tree Tree which contains the file.
 * @param file Will perform an action for all files in this file (including the
 * file itself). If the file is not a folder, the operation will succeed but it
 * will only be applied on the given file.
 * @param cb A callback that will be run for every item in the tree.
 * @param userdata Pointer to any additional data the user needs access to in
 * the callback.
 * @param recurse If should perform a recursive iteration (true) or just
 * perform the operation on all files contained in the given file (false).
 * @return Return code.
 */
swicc_ret_et swicc_disk_file_foreach(swicc_disk_tree_st *const tree,
                                     swicc_fs_file_st *const file,
                                     fs_file_foreach_cb *const cb,
                                     void *const userdata, bool const recurse);

/**
 * @brief Get a tree iterator for more efficient searches for trees.
 * @param disk
 * @param tree_iter Where to write the created tree iterator.
 * @return Return code.
 */
swicc_ret_et swicc_disk_tree_iter(swicc_disk_st const *const disk,
                                  swicc_disk_tree_iter_st *const tree_iter);

/**
 * @brief Move the head of the iterator forward.
 * @param tree_iter
 * @param tree Pointer to the next tree will be written here.
 * @return Return code.
 */
swicc_ret_et swicc_disk_tree_iter_next(swicc_disk_tree_iter_st *const tree_iter,
                                       swicc_disk_tree_st **const tree);

/**
 * @brief Iterate until a given tree index is found.
 * @param tree_iter
 * @param tree_idx
 * @param tree Pointer to the tree at the desired index will be written here.
 * @return Return code.
 * @note On failure, the iterator is left on the furthest reached element
 * (reached without any errors).
 * @note When some index n is reached, there is no way to go to index n - 1
 * without creating a new tree iterator, the current iterator will indicate that
 * the tree was not found.
 */
swicc_ret_et swicc_disk_tree_iter_idx(swicc_disk_tree_iter_st *const tree_iter,
                                      uint8_t const tree_idx,
                                      swicc_disk_tree_st **const tree);

/**
 * @brief Dealloc all disk buffers that hold forest data.
 * @param disk Disk for which to empty the forest/root.
 */
void swicc_disk_root_empty(swicc_disk_st *const disk);

/**
 * @brief Remove the SID LUT from a given tree.
 * @param tree The tree in which to empty the SID LUT.
 */
void swicc_disk_lutsid_empty(swicc_disk_tree_st *const tree);

/**
 * @brief Dealloc all disk buffers that hold ID LUT data.
 * @param disk Disk for which to empty the ID LUT.
 */
void swicc_disk_lutid_empty(swicc_disk_st *const disk);

/**
 * @brief Create the LUT for IDs on the disk.
 * @param disk
 * @return Return code.
 */
swicc_ret_et swicc_disk_lutid_rebuild(swicc_disk_st *const disk);

/**
 * @brief Create a LUT for SIDs for a tree.
 * @param disk
 * @param tree The tree for which to recreate the SID LUT.
 * @return Return code.
 */
swicc_ret_et swicc_disk_lutsid_rebuild(swicc_disk_st *const disk,
                                       swicc_disk_tree_st *const tree);

/**
 * @brief Perform a lookup in the SID LUT of a given tree.
 * @param tree
 * @param sid
 * @param file Gets the file header that was found with the lookup (only on
 * success).
 * @return Return code.
 */
swicc_ret_et swicc_disk_lutsid_lookup(swicc_disk_tree_st const *const tree,
                                      swicc_fs_sid_kt const sid,
                                      swicc_fs_file_st *const file);

/**
 * @brief Perform a lookup in the ID LUT of a given disk.
 * @param tree Gets a pointer to the tree in which the file is located (only on
 * success).
 * @param id
 * @param file Gets the file header that was found with the lookup (only on
 * success).
 * @return Return code.
 */
swicc_ret_et swicc_disk_lutid_lookup(swicc_disk_st const *const disk,
                                     swicc_disk_tree_st **const tree,
                                     swicc_fs_id_kt const id,
                                     swicc_fs_file_st *const file);

/**
 * @brief Obtain data contained in a record inside a file.
 * @param tree The tree which contains the file.
 * @param file The file which must contain the record.
 * @param idx Index of the record to obtain.
 * @param buf Where the pointer to the record buffer will be written.
 * @param len Length of the record buffer.
 * @return Return code.
 */
swicc_ret_et swicc_disk_file_rcrd(swicc_disk_tree_st const *const tree,
                                  swicc_fs_file_st const *const file,
                                  swicc_fs_rcrd_idx_kt const idx,
                                  uint8_t **const buf, uint8_t *const len);

/**
 * @brief Gets the number of records that a file holds.
 * @param tree The tree which contains the file.
 * @param file
 * @param rcrd_count Where the record count will be written.
 */
swicc_ret_et swicc_disk_file_rcrd_cnt(swicc_disk_tree_st const *const tree,
                                      swicc_fs_file_st const *const file,
                                      uint32_t *const rcrd_cnt);

/**
 * @brief Get the ADF/MF at the root of a tree.
 * @param tree
 * @param file_adf
 * @return Return code.
 * @note The file written into the given file struct is guaranteed to be an ADF
 * or MF on success.
 */
swicc_ret_et swicc_disk_tree_file_root(swicc_disk_tree_st const *const tree,
                                       swicc_fs_file_st *const file_root);

/**
 * @brief Get the parent file of a file.
 * @param tree Tree containing the file and the parent (parent can be the root
 * of this tree).
 * @param file File to find parent of.
 * @param file_parent Where the parent file will be written.
 * @return Return code.
 */
swicc_ret_et swicc_disk_tree_file_parent(swicc_disk_tree_st const *const tree,
                                         swicc_fs_file_st const *const file,
                                         swicc_fs_file_st *const file_parent);
