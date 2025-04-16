#pragma once

#include "bucket_storage_class.hpp"

#include <iterator>

template< typename T >
class BucketStorage< T >::abstract_iterator
{
  public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using size_type = typename BucketStorage< T >::size_type;

	abstract_iterator() : storage(nullptr), block(nullptr), index(0) {}

	bool operator==(const abstract_iterator& other) const
	{
		return storage == other.storage && block == other.block && index == other.index;
	}

	bool operator!=(const abstract_iterator& other) const { return !(*this == other); }

	bool operator<(const abstract_iterator& other) const
	{
		return (storage == other.storage && block == other.block && index < other.index) ||
			   (storage == other.storage && block != other.block && find_block_index(block) < find_block_index(other.block));
	}

	bool operator<=(const abstract_iterator& other) const { return *this < other || *this == other; }
	bool operator>(const abstract_iterator& other) const { return !(*this <= other); }
	bool operator>=(const abstract_iterator& other) const { return !(*this < other); }

  protected:
	BucketStorage* storage;
	typename BucketStorage::Block* block;
	size_type index;

	abstract_iterator(BucketStorage* storage, typename BucketStorage::Block* block, size_type index) :
		storage(storage), block(block), index(index)
	{
	}

	typename BucketStorage::Block* find_next_block(typename BucketStorage::Block* current_block) const
	{
		for (size_type i = 0; i < storage->m_blocks_count; ++i)
		{
			if (storage->m_blocks[i] == current_block && i + 1 < storage->m_blocks_count)
				return storage->m_blocks[i + 1];
		}
		return nullptr;
	}

	typename BucketStorage::Block* find_previous_block(typename BucketStorage::Block* current_block) const
	{
		for (size_type i = 0; i < storage->m_blocks_count; ++i)
		{
			if (storage->m_blocks[i] == current_block && i > 0)
				return storage->m_blocks[i - 1];
		}
		return nullptr;
	}

	typename BucketStorage::Block* find_last_active_block() const
	{
		for (size_type i = storage->m_blocks_count; i-- > 0;)
		{
			if (storage->m_blocks[i]->active_count > 0)
				return storage->m_blocks[i];
		}
		return nullptr;
	}

	size_type find_block_index(typename BucketStorage::Block* current_block) const
	{
		for (size_type i = 0; i < storage->m_blocks_count; ++i)
		{
			if (storage->m_blocks[i] == current_block)
				return i;
		}
		return storage->m_blocks_count;
	}

	void increment()
	{
		if (block == nullptr)
			return;

		do
		{
			++index;
			while (block != nullptr && index >= block->capacity)
			{
				block = find_next_block(block);
				index = 0;
			}
		} while (block != nullptr && block->m_activity[index] > 0);
	}

	void decrement()
	{
		if (block == nullptr)
		{
			block = find_last_active_block();
			if (block)
				index = block->capacity;
		}

		do
		{
			if (index == 0)
			{
				block = find_previous_block(block);
				index = block ? block->capacity : 0;
			}
			if (block)
				--index;
		} while (block && block->m_activity[index] > 0);
	}
};

template< typename T >
class BucketStorage< T >::iterator : public BucketStorage< T >::abstract_iterator
{
  public:
	using pointer = T*;
	using reference = T&;

	iterator() : abstract_iterator() {}

	reference operator*() const { return *reinterpret_cast< pointer >(&this->block->m_elements[this->index]); }
	pointer operator->() const { return reinterpret_cast< pointer >(&this->block->m_elements[this->index]); }

	iterator& operator++()
	{
		this->increment();
		return *this;
	}

	iterator operator++(int)
	{
		iterator tmp = *this;
		++(*this);
		return tmp;
	}

	iterator& operator--()
	{
		this->decrement();
		return *this;
	}

	iterator operator--(int)
	{
		iterator tmp = *this;
		--(*this);
		return tmp;
	}

  private:
	friend class BucketStorage< T >;
	iterator(BucketStorage* storage, typename BucketStorage::Block* block, size_type index) :
		abstract_iterator(storage, block, index)
	{
	}
};

template< typename T >
class BucketStorage< T >::const_iterator : public BucketStorage< T >::abstract_iterator
{
  public:
	using pointer = const T*;
	using reference = const T&;

	const_iterator() : abstract_iterator() {}

	reference operator*() const { return *reinterpret_cast< pointer >(&this->block->m_elements[this->index]); }
	pointer operator->() const { return reinterpret_cast< pointer >(&this->block->m_elements[this->index]); }

	const_iterator& operator++()
	{
		this->increment();
		return *this;
	}

	const_iterator operator++(int)
	{
		const_iterator tmp = *this;
		++(*this);
		return tmp;
	}

	const_iterator& operator--()
	{
		this->decrement();
		return *this;
	}

	const_iterator operator--(int)
	{
		const_iterator tmp = *this;
		--(*this);
		return tmp;
	}

  private:
	friend class BucketStorage< T >;
	const_iterator(const BucketStorage* storage, typename BucketStorage::Block* block, size_type index) :
		abstract_iterator(const_cast< BucketStorage* >(storage), block, index)
	{
	}
};
//
