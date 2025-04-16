#pragma once
#include "iterators.hpp"

#include <stdexcept>

template< typename T >
BucketStorage< T >::BucketStorage(size_type block_capacity) :
	m_blocks(nullptr), m_blocks_count(0), m_total_size(0), m_total_capacity(0), m_block_capacity_(block_capacity)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage& other) :
	m_blocks(nullptr), m_blocks_count(0), m_total_size(0), m_total_capacity(0), m_block_capacity_(other.m_block_capacity_)
{
	m_blocks = new BlockPtr[other.m_blocks_count];
	m_blocks_count = other.m_blocks_count;
	m_total_size = other.m_total_size;
	m_total_capacity = other.m_total_capacity;

	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		Block* other_block = other.m_blocks[i];
		Block* new_block = new Block(other_block->capacity);

		for (size_type j = 0; j < other_block->capacity; ++j)
		{
			if (other_block->m_activity[j] == 0)
				new (&new_block->m_elements[j]) T(other_block->m_elements[j]);
			new_block->m_activity[j] = other_block->m_activity[j];
		}
		new_block->active_count = other_block->active_count;
		m_blocks[i] = new_block;
	}
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage&& other) noexcept :
	m_blocks(other.m_blocks), m_blocks_count(other.m_blocks_count), m_total_size(other.m_total_size),
	m_total_capacity(other.m_total_capacity), m_block_capacity_(other.m_block_capacity_)
{
	other.m_blocks = nullptr;
	other.m_blocks_count = 0;
	other.m_total_size = 0;
	other.m_total_capacity = 0;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(const BucketStorage& other)
{
	if (this != &other)
	{
		BucketStorage tmp(other);
		swap(tmp);
	}
	return *this;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(BucketStorage&& other) noexcept
{
	if (this != &other)
	{
		BucketStorage< T > temp(std::move(other));
		swap(temp);
	}
	return *this;
}

template< typename T >
BucketStorage< T >::~BucketStorage()
{
	clear();
}

template< typename T >
template< typename U >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(U&& value)
{
	if (m_total_size == m_total_capacity)
		allocate_block();
	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		Block* block = m_blocks[i];
		for (size_type j = 0; j < block->capacity; ++j)
		{
			if (block->m_activity[j] > 0)
			{
				new (&block->m_elements[j]) T(std::forward< U >(value));
				block->m_activity[j] = 0;
				block->active_count++;
				m_total_size++;
				return iterator(this, block, j);
			}
		}
	}
	throw std::runtime_error("Failed to insert value.");
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(iterator it)
{
	if (it == end())
		throw std::out_of_range("Iterator out of range");

	Block* block = it.block;
	size_type index = it.index;

	if (block->m_activity[index] > 0)
		throw std::runtime_error("Element already erased");

	block->m_activity[index] = 1;
	block->m_elements[index].~T();
	block->active_count--;
	m_total_size--;

	if (block->active_count == 0)
	{
		deallocate_block(block);
		return end();
	}

	for (size_type i = index + 1; i < block->capacity; ++i)
	{
		if (block->m_activity[i] == 0)
			return iterator(this, block, i);
	}

	return end();
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return m_total_size == 0;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::size() const noexcept
{
	return m_total_size;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return m_total_capacity;
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage& other) noexcept
{
	std::swap(m_blocks, other.m_blocks);
	std::swap(m_blocks_count, other.m_blocks_count);
	std::swap(m_total_size, other.m_total_size);
	std::swap(m_total_capacity, other.m_total_capacity);
	std::swap(m_block_capacity_, other.m_block_capacity_);
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	BucketStorage< T > new_storage(m_block_capacity_);

	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		Block* block = m_blocks[i];
		for (size_type j = 0; j < block->capacity; ++j)
		{
			if (block->m_activity[j] == 0)
			{
				new_storage.insert(std::move(block->m_elements[j]));
				block->m_elements[j].~T();
			}
		}
	}

	swap(new_storage);
}

template< typename T >
void BucketStorage< T >::clear() noexcept
{
	for (size_type i = 0; i < m_blocks_count; ++i)
		delete m_blocks[i];
	delete[] m_blocks;
	m_blocks = nullptr;
	m_blocks_count = 0;
	m_total_size = 0;
	m_total_capacity = 0;
}

template< typename T >
void BucketStorage< T >::allocate_block()
{
	Block* new_block = new Block(m_block_capacity_);

	BlockPtr* new_blocks;
	try
	{
		new_blocks = new BlockPtr[m_blocks_count + 1];
	} catch (...)
	{
		delete new_block;
		throw;
	}

	for (size_type i = 0; i < m_blocks_count; ++i)
		new_blocks[i] = m_blocks[i];

	new_blocks[m_blocks_count] = new_block;
	delete[] m_blocks;

	m_blocks = new_blocks;
	m_blocks_count++;
	m_total_capacity += m_block_capacity_;
}

template< typename T >
void BucketStorage< T >::deallocate_block(Block* block)
{
	BlockPtr* new_blocks = new BlockPtr[m_blocks_count - 1];
	size_type new_blocks_count = 0;

	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		if (m_blocks[i] != block)
		{
			new_blocks[new_blocks_count] = m_blocks[i];
			new_blocks_count++;
		}
	}

	delete block;
	delete[] m_blocks;
	m_blocks = new_blocks;
	m_blocks_count = new_blocks_count;
	m_total_capacity -= m_block_capacity_;
}

template< typename T >
BucketStorage< T >::Block::Block(size_type capacity) :
	m_elements(nullptr), m_activity(nullptr), active_count(0), capacity(capacity)
{
	m_elements = static_cast< T* >(operator new[](capacity * sizeof(T)));
	m_activity = new size_type[capacity];
	for (size_type i = 0; i < capacity; ++i)
		m_activity[i] = 1;
}

template< typename T >
BucketStorage< T >::Block::~Block()
{
	for (size_type i = 0; i < capacity; ++i)
	{
		if (m_activity[i] == 0)
			m_elements[i].~T();
	}
	operator delete[](m_elements);
	delete[] m_activity;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::begin() noexcept
{
	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		Block* block = m_blocks[i];
		for (size_type j = 0; j < block->capacity; ++j)
		{
			if (block->m_activity[j] == 0)
				return iterator(this, block, j);
		}
	}
	return iterator(this, nullptr, 0);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::begin() const noexcept
{
	for (size_type i = 0; i < m_blocks_count; ++i)
	{
		Block* block = m_blocks[i];
		for (size_type j = 0; j < block->capacity; ++j)
		{
			if (block->m_activity[j] == 0)
				return const_iterator(this, block, j);
		}
	}
	return const_iterator(this, nullptr, 0);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cbegin() const noexcept
{
	return begin();
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::end() noexcept
{
	return iterator(this, nullptr, 0);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::end() const noexcept
{
	return const_iterator(this, nullptr, 0);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cend() const noexcept
{
	return end();
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_to_distance(iterator it, difference_type distance)
{
	while (distance != 0)
	{
		if (distance > 0)
		{
			++it;
			--distance;
		}
		else
		{
			--it;
			++distance;
		}
	}
	return it;
}
//
