#pragma once
#include <stdexcept>

template< typename T >
class BucketStorage
{
  public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	class abstract_iterator;
	class iterator;
	class const_iterator;

	explicit BucketStorage(size_type block_capacity = 64);
	BucketStorage(const BucketStorage& other);
	BucketStorage(BucketStorage&& other) noexcept;
	BucketStorage& operator=(const BucketStorage& other);
	BucketStorage& operator=(BucketStorage&& other) noexcept;
	~BucketStorage();

	template< typename IteratorType >
	IteratorType begin_impl() const noexcept;

	template< typename U >
	iterator insert(U&& value);

	iterator erase(iterator it);
	bool empty() const noexcept;
	size_type size() const noexcept;
	size_type capacity() const noexcept;
	void shrink_to_fit();
	void clear() noexcept;
	void swap(BucketStorage& other) noexcept;

	iterator begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;

	iterator get_to_distance(iterator it, difference_type distance);

  private:
	struct Block;
	using BlockPtr = Block*;

	struct Block
	{
		T* m_elements;
		size_type* m_activity;
		size_type active_count;
		size_type capacity;

		Block(size_type capacity);
		~Block();
	};

	BlockPtr* m_blocks;
	size_type m_blocks_count;
	size_type m_total_size;
	size_type m_total_capacity;
	size_type m_block_capacity_;

	void allocate_block();
	void deallocate_block(Block* block);
};
//
