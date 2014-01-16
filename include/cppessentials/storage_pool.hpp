
#ifndef STORAGE_POOL_HPP_
#define STORAGE_POOL_HPP_

#include <cppessentials/config.h>

#if !defined(CPPES_HAVE_CXX0X)
#warning "storage_pool is NOT used because no C++11 support detected"
#else

template<class T>
class storage_pool {

	static const size_t mBlockSize = sizeof(T);
	typedef union freelist_t {char data[mBlockSize];freelist_t* next;} freelist_t;

private:

	int mUsed;
	int mCapacity;
	freelist_t* mFreelist;

public:
	storage_pool()
	: mUsed(0)
	, mFreelist(0)
	, mCapacity(0) {}

	void add_block(void* block, size_t blocksize) {
	    const int numBlocks = blocksize/sizeof(freelist_t);
	    if(numBlocks > 0) {
	        freelist_t* newblock = (freelist_t*)block;
	        for(size_t i=0;i<numBlocks-1;i++){
	            newblock[i].next = &newblock[i+1];
	        }
	        newblock[numBlocks-1].next = mFreelist;
	        mFreelist = &newblock[0];
            mCapacity += numBlocks;
		}
	}

	bool full() {
		return mFreelist == NULL;
	}

	int numUsed() {
	    return mUsed;
	}

	int getCapacity() {
	    return mCapacity;
	}

	T* malloc() {

		if(full()){
			return 0;
		}

		T* retval = (T*)mFreelist;
		mFreelist = mFreelist->next;

		mUsed++;
		return retval;

	}

	void free(T* ptr) {

        if(mFreelist == NULL) {
            mFreelist = (freelist_t*)ptr;
            mFreelist->next = NULL;
        } else {
            freelist_t* next = mFreelist;
            mFreelist = (freelist_t*)ptr;
            mFreelist->next = next;
		}

		mUsed--;
	}
};

#endif /* !defined(CPPES_HAVE_CXX0X) */

#endif /* STORAGE_POOL_HPP_ */
