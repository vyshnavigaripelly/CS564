/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University
 * of Wisconsin-Madison.
 */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include "string.h"

#include "buffer.h"
#include "file.h"
#include "page.h"
#include "types.h"

namespace badgerdb {

/**
 * @brief Datatype enumeration type.
 */
enum Datatype { INTEGER = 0, DOUBLE = 1, STRING = 2 };

/**
 * @brief Scan operations enumeration. Passed to BTreeIndex::startScan() method.
 */
enum Operator {
  LT,  /* Less Than */
  LTE, /* Less Than or Equal to */
  GTE, /* Greater Than or Equal to */
  GT   /* Greater Than */
};

/**
 * @brief Number of key slots in B+Tree leaf for INTEGER key.
 */
//                                                  sibling ptr             key
//                                                  rid
const int INTARRAYLEAFSIZE =
    (Page::SIZE - sizeof(PageId)) / (sizeof(int) + sizeof(RecordId));

/**
 * @brief Number of key slots in B+Tree non-leaf for INTEGER key.
 */
//                                                     level     extra pageNo
//                                                     key       pageNo
const int INTARRAYNONLEAFSIZE = (Page::SIZE - sizeof(int) - sizeof(PageId)) /
                                (sizeof(int) + sizeof(PageId));

/**
 * @brief The meta page, which holds metadata for Index file, is always first
 * page of the btree index file and is cast to the following structure to store
 * or retrieve information from it. Contains the relation name for which the
 * index is created, the byte offset of the key value on which the index is
 * made, the type of the key and the page no of the root page. Root page starts
 * as page 2 but since a split can occur at the root the root page may get moved
 * up and get a new page no.
 */
struct IndexMetaInfo {
  /**
   * Name of base relation.
   */
  char relationName[20];

  /**
   * Offset of attribute, over which index is built, inside the record stored in
   * pages.
   */
  int attrByteOffset;

  /**
   * Type of the attribute over which index is built.
   */
  Datatype attrType;

  /**
   * Page number of root page of the B+ Tree inside the file index file.
   */
  PageId rootPageNo;
};

/*
Each node is a page, so once we read the page in we just cast the pointer to the
page to this struct and use it to access the parts These structures basically
are the format in which the information is stored in the pages for the index
file depending on what kind of node they are. The level memeber of each non leaf
structure seen below is set to 1 if the nodes at this level are just above the
leaf nodes. Otherwise set to 0.
*/

/**
 * @brief Structure for all non-leaf nodes when the key is of INTEGER type.
 */
struct NonLeafNodeInt {
  /**
   * Level of the node in the tree.
   */
  int level = 0;

  /**
   * Stores keys.
   */
  int keyArray[INTARRAYNONLEAFSIZE]{};

  /**
   * Stores page numbers of child pages which themselves are other non-leaf/leaf
   * nodes in the tree.
   */
  PageId pageNoArray[INTARRAYNONLEAFSIZE + 1]{};
};

/**
 * @brief Structure for all leaf nodes when the key is of INTEGER type.
 */

struct LeafNodeInt {
  int level = -1;

  /**
   * Stores keys.
   */
  int keyArray[INTARRAYLEAFSIZE]{};

  /**
   * Stores RecordIds.
   */
  RecordId ridArray[INTARRAYLEAFSIZE]{};

  /**
   * Page number of the leaf on the right side.
   * This linking of leaves allows to easily move from one leaf to the next leaf
   * during index scan.
   */
  PageId rightSibPageNo = 0;
};

/**
 * @brief BTreeIndex class. It implements a B+ Tree index on a single attribute
 * of a relation. This index supports only one scan at a time.
 */
class BTreeIndex {
 private:
  /**
   * File object for the index file.
   */
  File *file{};

  /**
   * Buffer Manager Instance.
   */
  BufMgr *bufMgr{};

  /**
   * Datatype of attribute over which index is built.
   */
  Datatype attributeType;

  /**
   * Offset of attribute, over which index is built, inside records.
   */
  int attrByteOffset{};

  // MEMBERS SPECIFIC TO SCANNING

  /**
   * True if an index scan has been started.
   */
  bool scanExecuting{};

  /**
   * Index of next entry to be scanned in current leaf being scanned.
   */
  int nextEntry{};

  /**
   * Page number of current page being scanned.
   */
  PageId currentPageNum{};

  /**
   * Current Page being scanned.
   */
  Page *currentPageData{};

  /**
   * Low INTEGER value for scan.
   */
  int lowValInt{};

  /**
   * High INTEGER value for scan.
   */
  int highValInt{};

  /**
   * Low Operator. Can only be GT(>) or GTE(>=).
   */
  Operator lowOp{GT};

  /**
   * High Operator. Can only be LT(<) or LTE(<=).
   */
  Operator highOp{LT};

  struct IndexMetaInfo indexMetaInfo {};

  /**
   * Alloc a page in the buffer for a leaf node
   *
   * @param newPageId the page number for the new node
   * @return a pointer to the new leaf node
   */
  LeafNodeInt *allocLeafNode(PageId &newPageId);

  /**
   * Alloca a page in the buffer for an internal node
   *
   * @param newPageId the page number for the new node
   * @return a pointer to the new internal node
   */
  NonLeafNodeInt *allocNonLeafNode(PageId &newPageId);

  /**
   * This method takes in a page and checks if the page stores a leaf node or
   * an internal node.
   *
   * Assumption: The level for leaf node is -1.
   *
   * @param page the page being checked
   * @return true if the page stores a leaf node
   *         false if the page stores an internal node
   */
  bool isLeaf(Page *page);

  /**
   * Checks if an internal node is full
   *
   * Assumption: All valid page numbers are nonzero.
   *
   * @param node an internal node
   * @return true if an internal node is full
   *         false if an internal node is not full
   */
  bool isNonLeafNodeFull(NonLeafNodeInt *node);

  /**
   * Checks if a leaf node is full
   *
   * Assumption: All valid records are nonzero.
   *
   * @param node a leaf node
   * @return true if a leaf node is full
   *         false if a leaf node is not full
   */
  bool isLeafNodeFull(LeafNodeInt *node);

  /**
   * Returns the number of records stored in the leaf node.
   *
   * Assumption:
   *             1. All records are continuously stored.
   *             2. All valid records are nonzero.
   *
   * @param node a leaf node
   * @return the number of records stored in the leaf node
   */
  int getLeafLen(LeafNodeInt *node);

  /**
   * Returns the number of records stored in the internal node.
   *
   * Assumption:
   *             1. All records are continuously stored.
   *             2. All valid page numbers are nonzero.
   *
   * @param node an internal node
   * @return the number of records stored in the internal node
   */
  int getNonLeafLen(NonLeafNodeInt *node);

  /**
   * Given an integer array, find the index of the first integer larger than
   * (or equal to) the given key.
   *
   * Assumption: The array is sorted.
   *
   * @param arr an interger array
   * @param len the length of the array
   * @param key the target key
   * @param includeKey whether the current key is included
   *
   * @return a. the index of the first integer larger than the given key if
   *            includeKey = false
   *         b. the index of the first integer larger than or equal to the
   *            given key if includeKey = true
   *         c. -1 if the key is not found till the end of array
   */
  int findArrayIndex(const int *arr, int len, int key, bool includeKey = true);

  /**
   * Find the index of the first key smaller than the given key
   *
   * Assumption:
   *             1. All records are continuously stored.
   *             2. All valid page numbers are nonzero.
   *             3. All keys are sorted in the node.
   *
   * @param node an internal node
   * @param key the key to find
   * @return the index of the first key smaller than the given key
   *         return the largest index if not found
   */
  int findIndexNonLeaf(NonLeafNodeInt *node, int key);

  /**
   * Find the insertaion index for a key in a leaf node
   *
   * Assumption:
   *             1. All records are continuously stored.
   *             2. All valid records are nonzero.
   *             3. All keys are sorted in the node.
   *
   * @param node a leaf node
   * @param key the key to be inserted
   *
   * @return the insertaion index for a key in a leaf node
   */
  int findInsertionIndexLeaf(LeafNodeInt *node, int key);

  /**
   * Find the index of the first key larger than the given key in the leaf node
   *
   * Assumption:
   *             1. All records are continuously stored.
   *             2. All valid records are nonzero.
   *             3. All keys are sorted in the node.
   *
   * @param node a leaf node
   * @param key the key to find
   * @param includeKey whether the current key is included
   *
   * @return a. the index of the first integer larger than the given key if
   *            includeKey = false
   *         b. the index of the first integer larger than or equal to the
   *            given key if includeKey = true
   *         c. -1 if the key is not found till the end of array
   */
  int findScanIndexLeaf(LeafNodeInt *node, int key, bool includeKey);

  /**
   * Inserts the given key-record pair into the leaf node at the given insertion
   * index.
   *
   * @param node a leaf node
   * @param i the insertion index
   * @param key the key of the key-record pair to be inserted
   * @param rid the record ID of the key-record pair to be inserted
   */
  void insertToLeafNode(LeafNodeInt *node, int i, int key, RecordId rid);

  /**
   * Inserts the given key-(page number) pair into the given leaf node at the
   * given index.
   *
   * @param n an internal node
   * @param i the insertion index
   * @param key the key of the key-(page number) pair
   * @param pid the page number of the key-(page number) pair
   */
  void insertToNonLeafNode(NonLeafNodeInt *n, int i, int key, PageId pid);

  /**
   * Splits a leaf node into two.
   * It moves the records after the given index in the node into the new node.
   *
   * @param node a pointer to the original node
   * @param newNode a pointer to the new node
   * @param index the index where the split occurs.
   */
  void splitLeafNode(LeafNodeInt *node, LeafNodeInt *newNode, int index);

  /**
   * Split the internal node by the given index. It moves the values stored in
   * the given node after the split index into a new internal node.
   *
   * @param node an internal node
   * @param i the index where the split occurs.
   * @param keepMidKey Whether the value at the index should be moved to the
   * parent internal node or not. If keepMidKey is true, then the pair at the
   * index does not need to be moved up and will be moved to the newly created
   *                   internal node.
   *
   * @return a pointer to the newly created internal node.
   */
  void splitNonLeafNode(NonLeafNodeInt *curr, NonLeafNodeInt *next, int i,
                        bool keepMidKey);

  /**
   * Create a new root with midVal, pid1 and pid2.
   *
   * @param midVal the first middle value of the new root
   * @param pid1 the first page number in the new root
   * @param pid2 the second page number in the new root
   *
   * @return the page id of the new root
   */
  PageId splitRoot(int midVal, PageId pid1, PageId pid2);

  /**
   * Insert the given key-(record id) pair into the given leaf node.
   *
   * @param origNode a leaf node
   * @param origPageId the page id of the page that stores the leaf node
   * @param key the key of the key-record pair
   * @param rid the record id of the key-record pair
   * @param midVal a reference to an integer in the parent node. If the
   * insertion requires a split in the leaf node, midVal is set to the smallest
   * element of the newly created node.
   *
   * @return The page number of the newly created page if insertion requires a
   *         split, or 0 if no new node is created.
   */
  PageId insertToLeafPage(Page *origPage, PageId origPageId, int key,
                          RecordId rid, int &midVal);

  /**
   * Recursively insert the given key-record pair into the subtree with the
   * given root node. If the root node requires a split, the page number of the
   * newly created node will be return
   *
   * @param origPageId page id of the page that stores the root node of the
   *        subtree.
   * @param key the key of the key-record pair to be inserted
   * @param rid the record ID of the key-record pair to be inserted
   * @param midVal a pointer to an integer value to be stored in the parent
   * node. If the insertion requires a split in the current level, midVal is set
   *        to the smallest key stored in the subtree pointed by the newly
   * created node.
   *
   * @return the page number of the newly created node if a split occurs, or 0
   *         otherwise.
   */
  PageId insert(PageId origPageId, int key, RecordId rid, int &midVal);

  /**
   * Change the currently scanning page to the next page pointed to by the
   * current page.
   * @param node the node stored in the currently scanning page.
   */
  void moveToNextPage(LeafNodeInt *node);

  /**
   * Recursively find the page id of the first element larger than or equal to
   * the lower bound given.
   */
  void setPageIdForScan();

  /**
   * Find the first element in the currently scanning page that is within the
   * given bound.
   */
  void setEntryIndexForScan();

  /**
   * Continue scanning the next entry. If the currently scanning entry is the
   * last element in this page, set the current scanning page to the next page.
   */
  void setNextEntry();

 public:
  /**
   * BTreeIndex Constructor.
   * Check to see if the corresponding index file exists. If so, open the
   * file. If not, create it and insert entries for every tuple in the base
   * relation using FileScan class.
   *
   * @param relationName        Name of file.
   * @param outIndexName        Return the name of index file.
   * @param bufMgrIn						Buffer
   * Manager Instance
   * @param attrByteOffset			Offset of attribute, over which
   * index is to be built, in the record
   * @param attrType						Datatype
   * of attribute over which index is built
   * @throws  BadIndexInfoException     If the index file already exists for
   * the corresponding attribute, but values in metapage(relationName,
   * attribute byte offset, attribute type etc.) do not match with values
   * received through constructor parameters.
   */
  BTreeIndex(const std::string &relationName, std::string &outIndexName,
             BufMgr *bufMgrIn, const int attrByteOffset,
             const Datatype attrType);

  /**
   * BTreeIndex Destructor.
   * End any initialized scan, flush index file, after unpinning any pinned
   * pages, from the buffer manager and delete file instance thereby closing the
   * index file. Destructor should not throw any exceptions. All exceptions
   * should be caught in here itself.
   * */
  ~BTreeIndex();

  /**
   * Insert a new entry using the pair <value,rid>.
   * Start from root to recursively find out the leaf to insert the entry in.
   *The insertion may cause splitting of leaf node. This splitting will require
   *addition of new leaf page number entry into the parent non-leaf, which may
   *in-turn get split. This may continue all the way upto the root causing the
   *root to get split. If root gets split, metapage needs to be changed
   *accordingly. Make sure to unpin pages as soon as you can.
   * @param key			Key to insert, pointer to integer/double/char
   *string
   * @param rid			Record ID of a record whose entry is getting
   *inserted into the index.
   **/
  const void insertEntry(const void *key, const RecordId rid);

  /**
   * Begin a filtered scan of the index.  For instance, if the method is called
   * using ("a",GT,"d",LTE) then we should seek all entries with a value
   * greater than "a" and less than or equal to "d".
   * If another scan is already executing, that needs to be ended here.
   * Set up all the variables for scan. Start from root to find out the leaf
   *page that contains the first RecordID that satisfies the scan parameters.
   *Keep that page pinned in the buffer pool.
   * @param lowVal	Low value of range, pointer to integer / double / char
   *string
   * @param lowOp		Low operator (GT/GTE)
   * @param highVal	High value of range, pointer to integer / double / char
   *string
   * @param highOp	High operator (LT/LTE)
   * @throws  BadOpcodesException If lowOp and highOp do not contain one of
   *their their expected values
   * @throws  BadScanrangeException If lowVal > highval
   * @throws  NoSuchKeyFoundException If there is no key in the B+ tree that
   *satisfies the scan criteria.
   **/
  const void startScan(const void *lowVal, const Operator lowOp,
                       const void *highVal, const Operator highOp);

  /**
   * Fetch the record id of the next index entry that matches the scan.
   * Return the next record from current page being scanned. If current page has
   *been scanned to its entirety, move on to the right sibling of current page,
   *if any exists, to start scanning that page. Make sure to unpin any pages
   *that are no longer required.
   * @param outRid	RecordId of next record found that satisfies the scan
   *criteria returned in this
   * @throws ScanNotInitializedException If no scan has been initialized.
   * @throws IndexScanCompletedException If no more records, satisfying the scan
   *criteria, are left to be scanned.
   **/
  const void scanNext(RecordId &outRid);  // returned record id

  /**
   * Terminate the current scan. Unpin any pinned pages. Reset scan specific
   *variables.
   * @throws ScanNotInitializedException If no scan has been initialized.
   **/
  const void endScan();
};
}  // namespace badgerdb
