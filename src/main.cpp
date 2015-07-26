#include "main.h"

bool operator==(const BOX &lhs, const BOX &rhs)
{
	return (&lhs == &rhs)
		|| ( (lhs.x == rhs.x)
		  && (lhs.y == rhs.y)
		  && (lhs.w == rhs.w)
		  && (lhs.h == rhs.h) );
}

bool operator!=(const BOX &lhs, const BOX &rhs)
{
	return !(lhs == rhs);
}

BOX::BOX(int32_t x_, int32_t y_, int32_t w_, int32_t h_) :
	x(x_),
	y(y_),
	w(w_),
	h(h_)
{ /* nothing to do here */ }

/// @brief normal assignment operator
BOX &BOX::operator= (const BOX &src)
{
	if (&src != this) {
		x = src.x;
		y = src.y;
		w = src.w;
		h = src.h;
	}
	return *this;
}

/// @brief rvalue reference assignment operator
BOX &BOX::operator= (const BOX &&src)
{
	x = src.x;
	y = src.y;
	w = src.w;
	h = src.h;
	return *this;
}

void BOX::set(int32_t x_, int32_t y_, int32_t w_, int32_t h_)
{
	x = x_;
	y = y_;
	w = w_;
	h = h_;
}


POINT_t::POINT_t(int32_t x_, int32_t y_) :
	x(x_),
	y(y_)
{ /* nothing to do here */ }

POINT_t &POINT_t::operator=( const POINT_t& src )
{
	if (&src != this) {
		x = src.x;
		y = src.y;
	}
	return *this;
}


// Safe ctor for WEAPON class
WEAPON::WEAPON()
{
	memset(desc, 0, sizeof(char) * MAX_ITEM_DESC_LEN + 1);
	memset(name, 0, sizeof(char) * MAX_ITEM_NAME_LEN + 1);
}


/// @brief Get the delay or 1 if delay is zero.
int32_t WEAPON::getDelayDiv() const
{
	return delay > 0 ? delay : 1;
}


/// @brief Get the current weapon description
const char* WEAPON::getDesc() const
{
	return desc;
}


/// @brief Get the current weapon name
const char* WEAPON::getName() const
{
	return name;
}


/// @brief Safely set a new weapon description
void WEAPON::setDesc(const char* desc_)
{
	if (strlen(desc_) > MAX_ITEM_DESC_LEN)
		fprintf(stderr, "Weapon description for \"%s\" truncated! (%d/%lu characters)\n",
		        name, MAX_ITEM_DESC_LEN, strlen(desc_));
	strncpy(desc, desc_, MAX_ITEM_DESC_LEN);
}


/// @brief Safely set a new weapon name
void WEAPON::setName(const char* name_)
{
	if (strlen(name_) > MAX_ITEM_NAME_LEN)
		fprintf(stderr, "Weapon name for \"%s\" truncated! (%d/%lu characters)\n",
		        name_, MAX_ITEM_NAME_LEN, strlen(name_));
	strncpy(name, name_, MAX_ITEM_NAME_LEN);
}


// Safe ctor for WEAPON class
ITEM::ITEM()
{
	memset(vals, 0, sizeof(double) * MAX_ITEMVALS);
	memset(desc, 0, sizeof(char)   * MAX_ITEM_DESC_LEN + 1);
	memset(name, 0, sizeof(char)   * MAX_ITEM_NAME_LEN + 1);
}


/// @brief Get the current item description
const char* ITEM::getDesc() const
{
	return desc;
}

/// @brief Get the current item name
const char* ITEM::getName() const
{
	return name;
}

/// @brief Safely set a new item description
void ITEM::setDesc(const char* desc_)
{
	if (strlen(desc_) > MAX_ITEM_DESC_LEN)
		fprintf(stderr, "Item description for \"%s\" truncated! (%d/%lu characters)\n",
				name, MAX_ITEM_DESC_LEN, strlen(desc_));
	strncpy(desc, desc_, MAX_ITEM_DESC_LEN);
}

/// @brief Safely set a new item name
void ITEM::setName(const char* name_)
{
	if (strlen(name_) > MAX_ITEM_NAME_LEN)
		fprintf(stderr, "Item name for \"%s\" truncated! (%d/%lu characters)\n",
		        name_, MAX_ITEM_NAME_LEN, strlen(name_));
	strncpy(name, name_, MAX_ITEM_NAME_LEN);
}
