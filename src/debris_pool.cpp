#include "debris_pool.h"
#include "main.h"

#include <cassert>

/// @brief sDebrisItem default constructor
sDebrisItem::sDebrisItem(int32_t diameter_, sDebrisItem* next_) :
	idx( (diameter_ / 2) - 1),
	next(next_)
{
	if (diameter_ && (idx >= 0) && (idx < 5)) {
		bmp = create_bitmap(diameter_ + 1, diameter_ + 1);
	}

	if (bmp && next) {
		prev = next->prev;
		next->prev = this;
		if (prev)
			prev->next = this;
	}

	if (bmp) {
		clear_to_color(bmp, PINK);
	} else
		// unusable...
		delete this;
}


/// @brief take this out of the list and free bmp data
sDebrisItem::~sDebrisItem()
{
	if (bmp)
		destroy_bitmap(bmp);
	bmp = nullptr;

	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	prev = nullptr;
	next = nullptr;
}


/// @brief create an empty pool with a set limit
sDebrisPool::sDebrisPool(int32_t limit_) :
	limit(limit_ * 5) // The limit is per debris size
{
	DEBUG_LOG_OBJ("Debris Pool", "Pool created with limit %d", limit);

	// memset initialization, Visual C++ 2013 can not do array initialization
	memset(avail,  0, sizeof(int32_t) * 5);
	memset(counts, 0, sizeof(int32_t) * 5);
	memset(heads,  0, sizeof(item_t*) * 5);
	memset(tails,  0, sizeof(item_t*) * 5);

	// Pre-create a third of the possible pool elements now,
	// so an evenly distributed base is given at any time.
	int32_t max_items = limit / 3 / 5;

	for (int32_t r = 1; r < 6; ++r) {
		for (int32_t i = 0; i < max_items; ++i)
			create_item(r);
	}
}


/// @brief destroy all pool items
sDebrisPool::~sDebrisPool()
{
	for (int32_t r = 0; r < 5; ++r) {
		while (heads[r]) {
			item_t* curr = heads[r];
			heads[r] = curr->next;
			delete curr;
			--counts[r];
			--count_all;
		}

		if (counts[r]) {
			cerr << "ERROR: debris pool count for radius " << (r + 1);
			cerr << " should be zero but is " << counts[r] << " !" << endl;
		}
	}
	if (count_all) {
		cerr << "ERROR: total debris pool count should be zero but is ";
		cerr << count_all << " !" << endl;
	}
}


/// @brief centralized creation, so it can be used by both the ctor and get_item()
sDebrisItem* sDebrisPool::create_item(int32_t radius)
{
	item_t* res = nullptr;

	if (count_all < limit) {

		int32_t idx = radius - 1;

		res = new sDebrisItem(radius * 2, heads[idx]);

		if (res) {
			// Insert item as new head
			heads[idx] = res;
			if (nullptr == tails[idx])
				tails[idx] = res;

			// Count the item
			++count_all;
			++counts[idx];
			++avail[idx];
			DEBUG_LOG_OBJ("Debris", "New maximum number: %d", count_all)
		}

	}

	return res;
}


/** @brief mark one item as unused.
  * Do not do this yourself, although it is a struct.
  * The pool needs to keep track of how many items are available.
**/
void sDebrisPool::free_item(item_t* item)
{
	if (item && !item->is_free) {
		// Reset item to PINK
		if (item->bmp)
			clear_to_color(item->bmp, PINK);

		// Mark item as being free
		item->is_free = true;
		++avail[item->idx];
	}
}


/** @brief get the next free item with the specified radius.
  *
  * If no item is available, a new one will be created unless the
  * current limit is reached.
**/
sDebrisItem* sDebrisPool::get_item (int32_t radius)
{
	assert( (radius > 0) && (radius < 6)
		&& "ERROR: Only radius in the range [1;5] supported!");

	if ( (radius < 1) || (radius > 5) )
		return nullptr;

	int32_t idx = radius - 1;

	// See whether an item is available:
	if (avail[idx]) {
		item_t* curr = heads[idx];
		while (curr && !curr->is_free)
			curr = curr->next;

		assert(curr && "ERROR: avail[idx] is not 0, but no item available!");

		if (curr) {
			--avail[idx];
			curr->is_free = false;
			return curr;
		} else {
			cerr << "No item for radius " << radius << " found, but ";
			cerr << avail[idx] << " should be available!" << endl;
			avail[idx] = 0;
		}
	}

	/* --- If this failed, or no items are available, create a new item --- */

	// Check whether the limit is hit already
	if (count_all >= limit) {
		/* If the limit is hit, this is the strategy:
		 * a) look which index has the most items
		 * b) Take the first free item and delete it
		 * -> Voilà, another item is available!
		 * <- no item free? Doesn't matter, we are busy now!
		 * ( Note : If the list with the most items has none free,
		 *          then it is safe to assume that none has. )
		 */

		DEBUG_LOG_OBJ("Debris Limit", "The limit of %d was reached", limit)

		int32_t hasMaxIdx = 0;

		for (int32_t i = 0; i < 5; ++i) {
			if ( (i != idx)
			  && avail[i]
			  && ( (counts[i] > counts[hasMaxIdx])
				|| !avail[hasMaxIdx]) )
				hasMaxIdx = i;
		}

		if (avail[hasMaxIdx]) {
			item_t* curr = heads[hasMaxIdx];
			while (curr && !curr->is_free)
				curr = curr->next;

			// If a free item is found, delete it!
			if (curr) {
				// fix head/tail if affected
				if (heads[hasMaxIdx] == curr)
					heads[hasMaxIdx] = curr->next;
				if (tails[hasMaxIdx] == curr)
					tails[hasMaxIdx] = curr->prev;
				delete curr;
				--avail[hasMaxIdx];
				--counts[hasMaxIdx];
				--count_all;
			} else {
				cerr << "No item for radius " << (hasMaxIdx + 1) << " found, but ";
				cerr << avail[hasMaxIdx] << " should be available!" << endl;
				avail[hasMaxIdx] = 0;
			}
		}

	} // End of limit reached resolving strategy

	// If the limit is not reached (or relieved) at this point,
	// a new item can be created
	item_t* new_item = create_item(radius);

	if (new_item) {
		// Note: create_item() has already raised counts[idx],
		//       count_all and avail[idx].
		--avail[idx];
		new_item->is_free = false;
	}

	return new_item;
}

