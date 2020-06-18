/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University
 * of Wisconsin-Madison.
 */

#include "btree.h"
#include <algorithm>
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/end_of_file_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "filescan.h"

using namespace std;

namespace badgerdb {

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #########################   Alloc Helper   ########################## //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Alloca a page in the buffer for an internal node
 *
 * @param newPageId the page number for the new node
 * @return a pointer to the new internal node
 */
NonLeafNodeInt *BTreeIndex::allocNonLeafNode(PageId &newPageId) {
  NonLeafNodeInt *newNode;
  bufMgr->allocPage(file, newPageId, (Page *&)newNode);
  memset(newNode, 0, Page::SIZE);
  return newNode;
}

/**
 * Alloc a page in the buffer for a leaf node
 *
 * @param newPageId the page number for the new node
 * @return a pointer to the new leaf node
 */
LeafNodeInt *BTreeIndex::allocLeafNode(PageId &newPageId) {
  LeafNodeInt *newNode = (LeafNodeInt *)allocNonLeafNode(newPageId);
  newNode->level = -1;
  return newNode;
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #########################   Constructor   ########################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Constructor
 *
 * The constructor first checks if the specified index ﬁle exists.
 * And index ﬁle name is constructed by concatenating the relational name with
 * the offset of the attribute over which the index is built.
 *
 * If the index ﬁle exists, the ﬁle is opened. Else, a new index ﬁle is created.
 *
 * @param relationName The name of the relation on which to build the index.
 * @param outIndexName The name of the index file.
 * @param bufMgrIn The instance of the global buffer manager.
 * @param attrByteOffset The byte offset of the attribute in the tuple on which
 * to build the index.
 * @param attrType The data type of the attribute we are indexing.
 */
BTreeIndex::BTreeIndex(const string &relationName, string &outIndexName,
                       BufMgr *bufMgrIn, const int attrByteOffset_,
                       const Datatype attrType) {
  bufMgr = bufMgrIn;
  attrByteOffset = attrByteOffset_;
  attributeType = attrType;

  ostringstream idx_str{};
  idx_str << relationName << ',' << attrByteOffset;
  outIndexName = idx_str.str();

  relationName.copy(indexMetaInfo.relationName, 20, 0);
  indexMetaInfo.attrByteOffset = attrByteOffset;
  indexMetaInfo.attrType = attrType;

  file = new BlobFile(outIndexName, true);

  allocLeafNode(indexMetaInfo.rootPageNo);
  bufMgr->unPinPage(file, indexMetaInfo.rootPageNo, true);

  FileScan fscan(relationName, bufMgr);
  try {
    RecordId scanRid;
    while (1) {
      fscan.scanNext(scanRid);
      std::string recordStr = fscan.getRecord();
      const char *record = recordStr.c_str();
      int key = *((int *)(record + attrByteOffset));
      insertEntry(&key, scanRid);
    }
  } catch (EndOfFileException e) {
  }
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #######################  Generic Node Helper  ####################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

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
bool BTreeIndex::isLeaf(Page *page) { return *((int *)page) == -1; }

/**
 * Checks if an internal node is full
 *
 * Assumption: All valid page numbers are nonzero.
 *
 * @param node an internal node
 * @return true if an internal node is full
 *         false if an internal node is not full
 */
bool BTreeIndex::isNonLeafNodeFull(NonLeafNodeInt *node) {
  return node->pageNoArray[INTARRAYNONLEAFSIZE] != 0;
}

/**
 * Checks if a leaf node is full
 *
 * Assumption: All valid records are nonzero.
 *
 * @param node a leaf node
 * @return true if a leaf node is full
 *         false if a leaf node is not full
 */
bool BTreeIndex::isLeafNodeFull(LeafNodeInt *node) {
  return !(node->ridArray[INTARRAYLEAFSIZE - 1].page_number == 0 &&
           node->ridArray[INTARRAYLEAFSIZE - 1].slot_number == 0);
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #######################   Find Index Helper   ####################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Returns the number of records stored in the leaf node.
 *
 * Assumption: 1. All records are continuously stored.
 *             2. All valid records are nonzero.
 *
 * @param node a leaf node
 * @return the number of records stored in the leaf node
 */
int BTreeIndex::getLeafLen(LeafNodeInt *node) {
  static auto comp = [](const RecordId &r1, const RecordId &r2) {
    return r1.page_number > r2.page_number;
  };
  static RecordId emptyRecord{};

  RecordId *start = node->ridArray;
  RecordId *end = &node->ridArray[INTARRAYLEAFSIZE];

  return lower_bound(start, end, emptyRecord, comp) - start;
}

/**
 * Returns the number of records stored in the internal node.
 *
 * Assumption: 1. All records are continuously stored.
 *             2. All valid page numbers are nonzero.
 *
 * @param node an internal node
 * @return the number of records stored in the internal node
 */
int BTreeIndex::getNonLeafLen(NonLeafNodeInt *node) {
  static auto comp = [](const PageId &p1, const PageId &p2) { return p1 > p2; };
  PageId *start = node->pageNoArray;
  PageId *end = &node->pageNoArray[INTARRAYNONLEAFSIZE + 1];
  return lower_bound(start, end, 0, comp) - start;
}

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
int BTreeIndex::findArrayIndex(const int *arr, int len, int key,
                               bool includeKey) {
  if (!includeKey) key++;
  int result = lower_bound(arr, &arr[len], key) - arr;
  return result >= len ? -1 : result;
}

/**
 * Find the index of the first key smaller than the given key
 *
 * Assumption: 1. All records are continuously stored.
 *             2. All valid page numbers are nonzero.
 *             3. All keys are sorted in the node.
 *
 * @param node an internal node
 * @param key the key to find
 * @return the index of the first key smaller than the given key
 *         return the largest index if not found
 */
int BTreeIndex::findIndexNonLeaf(NonLeafNodeInt *node, int key) {
  int len = getNonLeafLen(node);
  int result = findArrayIndex(node->keyArray, len - 1, key);
  return result == -1 ? len - 1 : result;
}

/**
 * Find the insertaion index for a key in a leaf node
 *
 * Assumption: 1. All records are continuously stored.
 *             2. All valid records are nonzero.
 *             3. All keys are sorted in the node.
 *
 * @param node a leaf node
 * @param key the key to be inserted
 *
 * @return the insertaion index for a key in a leaf node
 */
int BTreeIndex::findInsertionIndexLeaf(LeafNodeInt *node, int key) {
  int len = getLeafLen(node);
  int result = findArrayIndex(node->keyArray, len, key);
  return result == -1 ? len : result;
}

/**
 * Find the index of the first key larger than the given key in the leaf node
 *
 * Assumption: 1. All records are continuously stored.
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
int BTreeIndex::findScanIndexLeaf(LeafNodeInt *node, int key, bool includeKey) {
  return findArrayIndex(node->keyArray, getLeafLen(node), key, includeKey);
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #########################   Insert Helper   ######################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Inserts the given key-record pair into the leaf node at the given insertion
 * index.
 *
 * @param node a leaf node
 * @param i the insertion index
 * @param key the key of the key-record pair to be inserted
 * @param rid the record ID of the key-record pair to be inserted
 */
void BTreeIndex::insertToLeafNode(LeafNodeInt *node, int i, int key,
                                  RecordId rid) {
  const size_t len = INTARRAYLEAFSIZE - i - 1;

  // shift items to add space for the new element
  memmove(&node->keyArray[i + 1], &node->keyArray[i], len * sizeof(int));
  memmove(&node->ridArray[i + 1], &node->ridArray[i], len * sizeof(RecordId));

  // save the key and record id to the leaf node
  node->keyArray[i] = key;
  node->ridArray[i] = rid;
}

/**
 * Inserts the given key-(page number) pair into the given leaf node at the
 * given index.
 *
 * @param n an internal node
 * @param i the insertion index
 * @param key the key of the key-(page number) pair
 * @param pid the page number of the key-(page number) pair
 */
void BTreeIndex::insertToNonLeafNode(NonLeafNodeInt *n, int i, int key,
                                     PageId pid) {
  const size_t len = INTARRAYNONLEAFSIZE - i - 1;

  // shift items to add space for the new element
  memmove(&n->keyArray[i + 1], &n->keyArray[i], len * sizeof(int));
  memmove(&n->pageNoArray[i + 2], &n->pageNoArray[i + 1], len * sizeof(PageId));

  // store the key and page number to the node
  n->keyArray[i] = key;
  n->pageNoArray[i + 1] = pid;
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// ##########################   Split Helper   ######################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Splits a leaf node into two.
 * It moves the records after the given index in the node into the new node.
 *
 * @param node a pointer to the original node
 * @param newNode a pointer to the new node
 * @param index the index where the split occurs.
 */
void BTreeIndex::splitLeafNode(LeafNodeInt *node, LeafNodeInt *newNode,
                               int index) {
  const size_t len = INTARRAYLEAFSIZE - index;

  // copy elements from old node to new node
  memcpy(&newNode->keyArray, &node->keyArray[index], len * sizeof(int));
  memcpy(&newNode->ridArray, &node->ridArray[index], len * sizeof(RecordId));

  // remove elements from old node
  memset(&node->keyArray[index], 0, len * sizeof(int));
  memset(&node->ridArray[index], 0, len * sizeof(RecordId));
}

/**
 * Split the internal node by the given index. It moves the values stored in the
 * given node after the split index into a new internal node.
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
void BTreeIndex::splitNonLeafNode(NonLeafNodeInt *curr, NonLeafNodeInt *next,
                                  int i, bool keepMidKey) {
  size_t len = INTARRAYNONLEAFSIZE - i;

  // copy keys from old node to new node
  if (keepMidKey)
    memcpy(&next->keyArray, &curr->keyArray[i], len * sizeof(int));
  else
    memcpy(&next->keyArray, &curr->keyArray[i + 1], (len - 1) * sizeof(int));

  // copy values from old node to new node
  memcpy(&next->pageNoArray, &curr->pageNoArray[i + 1], len * sizeof(PageId));

  // remove elements from old node
  memset(&curr->keyArray[i], 0, len * sizeof(int));
  memset(&curr->pageNoArray[i + 1], 0, len * sizeof(PageId));
}

/**
 * Create a new root with midVal, pid1 and pid2.
 *
 * @param midVal the first middle value of the new root
 * @param pid1 the first page number in the new root
 * @param pid2 the second page number in the new root
 *
 * @return the page id of the new root
 */
PageId BTreeIndex::splitRoot(int midVal, PageId pid1, PageId pid2) {
  // alloc a new page for root
  PageId newRootPageId;
  NonLeafNodeInt *newRoot = allocNonLeafNode(newRootPageId);

  // set key and page numbers
  newRoot->keyArray[0] = midVal;
  newRoot->pageNoArray[0] = pid1;
  newRoot->pageNoArray[1] = pid2;

  // unpin the root page
  bufMgr->unPinPage(file, newRootPageId, true);

  return newRootPageId;
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #########################       Insert      ######################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Insert the given key-(record id) pair into the given leaf node.
 *
 * @param origNode a leaf node
 * @param origPageId the page id of the page that stores the leaf node
 * @param key the key of the key-record pair
 * @param rid the record id of the key-record pair
 * @param midVal a reference to an integer in the parent node. If the insertion
 *               requires a split in the leaf node, midVal is set to the
 *               smallest element of the newly created node.
 *
 * @return The page number of the newly created page if insertion requires a
 *         split, or 0 if no new node is created.
 */
PageId BTreeIndex::insertToLeafPage(Page *origPage, PageId origPageId, int key,
                                    RecordId rid, int &midVal) {
  LeafNodeInt *origNode = (LeafNodeInt *)origPage;

  // finde the insertion index
  int index = findInsertionIndexLeaf(origNode, key);

  // if not full, directly insert the key and record id to the node
  if (!isLeafNodeFull(origNode)) {
    insertToLeafNode(origNode, index, key, rid);
    bufMgr->unPinPage(file, origPageId, true);
    return 0;
  }

  // the node is full at this point

  // the middle index for spliting the page
  const int middleIndex = INTARRAYLEAFSIZE / 2;

  // whether the new element is insert to the left half of the original node
  bool insertToLeft = index < middleIndex;

  // alloc a page for the new node
  PageId newPageId;
  LeafNodeInt *newNode = allocLeafNode(newPageId);

  // split the node to origNode and newNode
  splitLeafNode(origNode, newNode, middleIndex + insertToLeft);

  // insert the key and record id to the node
  if (insertToLeft)
    insertToLeafNode(origNode, index, key, rid);
  else
    insertToLeafNode(newNode, index - middleIndex, key, rid);

  // set the next page id
  newNode->rightSibPageNo = origNode->rightSibPageNo;
  origNode->rightSibPageNo = newPageId;

  // unpin the new node and the original node
  bufMgr->unPinPage(file, origPageId, true);
  bufMgr->unPinPage(file, newPageId, true);

  // set the middle value
  midVal = newNode->keyArray[0];
  return newPageId;
}

/**
 * Recursively insert the given key-record pair into the subtree with the given
 * root node. If the root node requires a split, the page number of the newly
 * created node will be return
 *
 * @param origPageId page id of the page that stores the root node of the
 *        subtree.
 * @param key the key of the key-record pair to be inserted
 * @param rid the record ID of the key-record pair to be inserted
 * @param midVal a pointer to an integer value to be stored in the parent node.
 *        If the insertion requires a split in the current level, midVal is set
 *        to the smallest key stored in the subtree pointed by the newly created
 *        node.
 *
 * @return the page number of the newly created node if a split occurs, or 0
 *         otherwise.
 */
PageId BTreeIndex::insert(PageId origPageId, int key, RecordId rid,
                          int &midVal) {
  Page *origPage;
  bufMgr->readPage(file, origPageId, origPage);

  if (isLeaf(origPage))  // base case
    return insertToLeafPage(origPage, origPageId, key, rid, midVal);

  NonLeafNodeInt *origNode = (NonLeafNodeInt *)origPage;

  // find the child page id
  int origChildPageIndex = findIndexNonLeaf(origNode, key);
  PageId origChildPageId = origNode->pageNoArray[origChildPageIndex];

  // insert key, rid to child and check whether child is splitted
  int newChildMidVal;
  PageId newChildPageId = insert(origChildPageId, key, rid, newChildMidVal);

  // not split in child
  if (newChildPageId == 0) {
    bufMgr->unPinPage(file, origPageId, false);
    return 0;
  }

  // split in child, need to add splitted child to currNode
  int index = findIndexNonLeaf(origNode, newChildMidVal);
  if (!isNonLeafNodeFull(origNode)) {  // current node is not full
    insertToNonLeafNode(origNode, index, newChildMidVal, newChildPageId);
    bufMgr->unPinPage(file, origPageId, true);
    return 0;
  }

  // the middle index for spliting the page
  int middleIndex = (INTARRAYNONLEAFSIZE - 1) / 2;

  // whether the new element is insert to the left half of the original node
  bool insertToLeft = index < middleIndex;

  // split
  int splitIndex = middleIndex + insertToLeft;
  int insertIndex = insertToLeft ? index : index - middleIndex;

  // insert to right[0]
  bool moveKeyUp = !insertToLeft && insertIndex == 0;

  // if we need to move key up, set midVal = key, else key at splited index
  midVal = moveKeyUp ? newChildMidVal : origNode->keyArray[splitIndex];

  // alloc a page for the new node
  PageId newPageId;
  NonLeafNodeInt *newNode = allocNonLeafNode(newPageId);

  // split the node to origNode and newNode
  splitNonLeafNode(origNode, newNode, splitIndex, moveKeyUp);

  // need to insert
  if (!moveKeyUp) {
    NonLeafNodeInt *node = insertToLeft ? origNode : newNode;
    insertToNonLeafNode(node, insertIndex, newChildMidVal, newChildPageId);
  }

  // write the page back
  bufMgr->unPinPage(file, origPageId, true);
  bufMgr->unPinPage(file, newPageId, true);

  // return new page
  return newPageId;
}

/**
 * Insert a new entry using the pair <value,rid>.
 * Start from root to recursively find out the leaf to insert the entry in.
 * The insertion may cause splitting of leaf node. This splitting will require
 * addition of new leaf page number entry into the parent non-leaf, which may
 * in-turn get split. This may continue all the way upto the root causing the
 * root to get split. If root gets split, metapage needs to be changed
 * accordingly. Make sure to unpin pages as soon as you can.
 * @param key			Key to insert, pointer to integer/double/char
 *string
 * @param rid			Record ID of a record whose entry is getting
 *inserted into the index.
 **/
const void BTreeIndex::insertEntry(const void *key, const RecordId rid) {
  int midval;
  PageId pid = insert(indexMetaInfo.rootPageNo, *(int *)key, rid, midval);

  if (pid != 0)
    indexMetaInfo.rootPageNo = splitRoot(midval, indexMetaInfo.rootPageNo, pid);
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// #######################        Scan        ########################## //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Change the currently scanning page to the next page pointed to by the current
 * page.
 * @param node the node stored in the currently scanning page.
 */
void BTreeIndex::moveToNextPage(LeafNodeInt *node) {
  bufMgr->unPinPage(file, currentPageNum, false);
  currentPageNum = node->rightSibPageNo;
  bufMgr->readPage(file, currentPageNum, currentPageData);
  nextEntry = 0;
}

/**
 * Recursively find the page id of the first element larger than or equal to the
 * lower bound given.
 */
void BTreeIndex::setPageIdForScan() {
  bufMgr->readPage(file, currentPageNum, currentPageData);
  if (isLeaf(currentPageData)) return;

  NonLeafNodeInt *node = (NonLeafNodeInt *)currentPageData;

  bufMgr->unPinPage(file, currentPageNum, false);
  currentPageNum = node->pageNoArray[findIndexNonLeaf(node, lowValInt)];
  setPageIdForScan();
}

/**
 * Find the first element in the currently scanning page that is within the
 * given bound.
 */
void BTreeIndex::setEntryIndexForScan() {
  LeafNodeInt *node = (LeafNodeInt *)currentPageData;
  int entryIndex = findScanIndexLeaf(node, lowValInt, lowOp == GTE);
  if (entryIndex == -1)
    moveToNextPage(node);
  else
    nextEntry = entryIndex;
}

/**
 *
 * This method is used to begin a filtered scan” of the index.
 *
 * For example, if the method is called using arguments (1,GT,100,LTE), then
 * the scan should seek all entries greater than 1 and less than or equal to
 * 100.
 *
 * @param lowValParm The low value to be tested.
 * @param lowOpParm The operation to be used in testing the low range.
 * @param highValParm The high value to be tested.
 * @param highOpParm The operation to be used in testing the high range.
 */
const void BTreeIndex::startScan(const void *lowValParm,
                                 const Operator lowOpParm,
                                 const void *highValParm,
                                 const Operator highOpParm) {
  if (lowOpParm != GT && lowOpParm != GTE) throw BadOpcodesException();
  if (highOpParm != LT && highOpParm != LTE) throw BadOpcodesException();

  lowValInt = *((int *)lowValParm);
  highValInt = *((int *)highValParm);
  if (lowValInt > highValInt) throw BadScanrangeException();

  lowOp = lowOpParm;
  highOp = highOpParm;

  scanExecuting = true;

  currentPageNum = indexMetaInfo.rootPageNo;

  setPageIdForScan();
  setEntryIndexForScan();

  LeafNodeInt *node = (LeafNodeInt *)currentPageData;
  RecordId outRid = node->ridArray[nextEntry];
  if ((outRid.page_number == 0 && outRid.slot_number == 0) ||
      node->keyArray[nextEntry] > highValInt ||
      (node->keyArray[nextEntry] == highValInt && highOp == LT)) {
    endScan();
    throw NoSuchKeyFoundException();
  }
}

/**
 * Continue scanning the next entry. If the currently scanning entry is the last
 * element in this page, set the current scanning page to the next page.
 */
void BTreeIndex::setNextEntry() {
  nextEntry++;
  LeafNodeInt *node = (LeafNodeInt *)currentPageData;
  if (nextEntry >= INTARRAYLEAFSIZE ||
      node->ridArray[nextEntry].page_number == 0) {
    moveToNextPage(node);
  }
}

/**
 * This method fetches the record id of the next tuple that matches the scan
 * criteria. If the scan has reached the end, then it should throw the
 * following exception: IndexScanCompletedException.
 *
 * For instance, if there are two data entries that need to be returned in a
 * scan, then the third call to scanNext must throw
 * IndexScanCompletedException. A leaf page that has been read into the buffer
 * pool for the purpose of scanning, should not be unpinned from buffer pool
 * unless all records from it are read or the scan has reached its end. Use
 * the right sibling page number value from the current leaf to move on to the
 * next leaf which holds successive key values for the scan.
 *
 * @param outRid An output value
 *               This is the record id of the next entry that matches the scan
 * filter set in startScan.
 */
const void BTreeIndex::scanNext(RecordId &outRid) {
  if (!scanExecuting) throw ScanNotInitializedException();

  LeafNodeInt *node = (LeafNodeInt *)currentPageData;
  outRid = node->ridArray[nextEntry];
  int val = node->keyArray[nextEntry];

  if ((outRid.page_number == 0 &&
       outRid.slot_number == 0) ||            // current record ID is empty
      val > highValInt ||                     // value is out of range
      (val == highValInt && highOp == LT)) {  // value reaches the higher end
    throw IndexScanCompletedException();
  }
  setNextEntry();
}

/**
 * This method terminates the current scan and unpins all the pages that have
 * been pinned for the purpose of the scan.
 *
 * It throws ScanNotInitializedException when called before a successful
 * startScan call.
 */
const void BTreeIndex::endScan() {
  if (!scanExecuting) throw ScanNotInitializedException();
  scanExecuting = false;
  bufMgr->unPinPage(file, currentPageNum, false);
}

// ##################################################################### //
// ##################################################################### //
// ##################################################################### //
// ##########################   Destructor   ########################### //
// ##################################################################### //
// ##################################################################### //
// ##################################################################### //

/**
 * Destructor
 *
 * Perform any cleanup that may be necessary, including
 *      clearing up any state variables,
 *      unpinning any B+ Tree pages that are pinned, and
 *      flushing the index file (by calling bufMgr->flushFile()).
 *
 * Note that this method does not delete the index file! But, deletion of the
 * file object is required, which will call the destructor of File class causing
 * the index file to be closed.
 */
BTreeIndex::~BTreeIndex() {
  if (scanExecuting) endScan();
  bufMgr->flushFile(file);
  delete file;
}

}  // namespace badgerdb